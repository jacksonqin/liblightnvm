/*
 * addr - Sector addressing functions for write, read, and meta-data print
 *        Block addressing functions for erase and mark
 *
 * Copyright (C) 2015 Javier González <javier@cnexlabs.com>
 * Copyright (C) 2015 Matias Bjørling <matias@cnexlabs.com>
 * Copyright (C) 2016 Simon A. F. Lund <slund@cnexlabs.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  - Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <linux/lightnvm.h>
#include <liblightnvm.h>
#include <nvm.h>
#include <nvm_debug.h>

void nvm_ret_pr(NVM_RET *ret)
{
	printf("nvm_ret { result(%u), status(%lu) }\n", ret->result,
	       ret->status);
}

/**
 * Convert nvm_address from generic format to device specific format
 *
 * @param dev The device which address format to convert to
 * @param addr The address to convert
 * @returns Address formatted to device
 */
static inline struct nvm_addr nvm_addr_gen2dev(struct nvm_dev *dev,
					       struct nvm_addr addr)
{
	struct nvm_addr d_addr;

	d_addr.ppa = 0;
	d_addr.ppa |= ((uint64_t)addr.g.ch) << dev->fmt.n.ch_ofz;
	d_addr.ppa |= ((uint64_t)addr.g.lun) << dev->fmt.n.lun_ofz;
	d_addr.ppa |= ((uint64_t)addr.g.pl) << dev->fmt.n.pl_ofz;
	d_addr.ppa |= ((uint64_t)addr.g.blk) << dev->fmt.n.blk_ofz;
	d_addr.ppa |= ((uint64_t)addr.g.pg) << dev->fmt.n.pg_ofz;
	d_addr.ppa |= ((uint64_t)addr.g.sec) << dev->fmt.n.sec_ofz;

	return d_addr;
}

static ssize_t
nvm_addr_cmd(struct nvm_dev *dev, struct nvm_addr addrs[], int len, void *data,
             void *meta, uint16_t flags, uint16_t opcode,
             struct nvm_return *ret)
{
	struct nvm_user_vio ctl;
	struct nvm_addr dev_addrs[len];
	int i, err;

	memset(&ctl, 0, sizeof(ctl));
	ctl.opcode = opcode;
	ctl.control = flags | NVM_MAGIC_FLAG_DEFAULT;

	// Setup PPAs: Convert generic address format to device specific
	for (i = 0; i < len; ++i) {
		dev_addrs[i] = nvm_addr_gen2dev(dev, addrs[i]);
	}
	ctl.nppas = len - 1;	// unnatural numbers, we count from zero
	ctl.ppa_list = len == 1 ? dev_addrs[0].ppa : (uint64_t)dev_addrs;

	// Setup data
	ctl.addr = (uint64_t)data;
	ctl.data_len = data ? dev->geo.nbytes * len : 0;

	// Setup metadata
	ctl.metadata = (uint64_t)meta;
	ctl.metadata_len = meta ? dev->geo.meta_nbytes * len : 0;

	err = ioctl(dev->fd, NVME_NVM_IOCTL_SUBMIT_VIO, &ctl);
#ifdef NVM_DEBUG_ENABLED
	if (err || ctl.result || ctl.status) {
		int i;

		NVM_DEBUG("err(%d), ctl: result(0x%x), status(%llu), nppas(%d)",
			  err, ctl.result, ctl.status, ctl.nppas);
		for (i = 0; i < len; ++i)
			nvm_addr_pr(addrs[i]);
	}
#endif
	if (ret) {
		ret->result = ctl.result;
		ret->status = ctl.status;
	}
	if (err) {	// Give up on IOCTL errors
		errno = EIO;
		return -1;
	}

	switch (ctl.result) {
	case 0x0:	// All good
		return 0;
	case 0x4700:	// As good as it gets..
		return 0;

	default:	// Everything else is an error
		errno = EIO;
		return -1;
	}
}

ssize_t nvm_addr_erase(NVM_DEV dev, NVM_ADDR addrs[], int naddrs, uint16_t flags,
		       NVM_RET *ret)
{
	return nvm_addr_cmd(dev, addrs, naddrs, NULL, NULL, flags,
			    NVM_MAGIC_OPCODE_ERASE, ret);
}

ssize_t nvm_addr_write(struct nvm_dev *dev, NVM_ADDR addrs[], int naddrs,
		       const void *data, const void *meta, uint16_t flags,
		       NVM_RET *ret)
{
	char *cdata = (char *)data;
        char *cmeta = (char *)meta;

	return nvm_addr_cmd(dev, addrs, naddrs, cdata, cmeta, flags,
			    NVM_MAGIC_OPCODE_WRITE, ret);
}

ssize_t nvm_addr_read(struct nvm_dev *dev, NVM_ADDR addrs[], int naddrs,
		      void *data, void *meta, uint16_t flags, NVM_RET *ret)
{
	return nvm_addr_cmd(dev, addrs, naddrs, data, meta, flags,
			    NVM_MAGIC_OPCODE_READ, ret);
}

ssize_t nvm_addr_mark(NVM_DEV dev, NVM_ADDR addrs[], int naddrs, uint16_t flags,
		      NVM_RET *ret)
{
	switch (flags) {
	case 0x0:
	case 0x1:
	case 0x2:
		break;
	default:
		errno = EINVAL;
		return -1;
	}

	return nvm_addr_cmd(dev, addrs, naddrs, NULL, NULL, flags, 0xF1, ret);
}

void nvm_addr_fmt_pr(struct nvm_addr_fmt *fmt)
{
	printf("fmt {\n");
	printf(" ch_ofz(%d), ch_len(%d)", fmt->n.ch_ofz, fmt->n.ch_len);
	printf(",\n lun_ofz(%d), lun_len(%d)", fmt->n.lun_ofz, fmt->n.lun_len);
	printf(",\n pl_ofz(%d), pl_len(%d)", fmt->n.pl_ofz, fmt->n.pl_len);
	printf(",\n blk_ofz(%d), blk_len(%d)", fmt->n.blk_ofz, fmt->n.blk_len);
	printf(",\n pg_ofz(%d), pg_len(%d)", fmt->n.pg_ofz, fmt->n.pg_len);
	printf(",\n sec_ofz(%d), sec_len(%d)", fmt->n.sec_ofz, fmt->n.sec_len);
	printf("\n}\n");
}

void nvm_addr_pr(struct nvm_addr addr)
{
	printf("(%016lx){ ch(%02d), lun(%02d), pl(%d), ",
	       addr.ppa, addr.g.ch, addr.g.lun, addr.g.pl);
	printf("blk(%04d), pg(%03d), sec(%d) }\n",
	       addr.g.blk, addr.g.pg, addr.g.sec);
}
