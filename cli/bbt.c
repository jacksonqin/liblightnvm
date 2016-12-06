/**
 * bbt - CLI example for bad-block-table
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <liblightnvm.h>

#define CLI_CMD_LEN 50

ssize_t get(NVM_DEV dev, NVM_GEO geo, NVM_ADDR list[], int len, int flags)
{
	return 0;
}

ssize_t set(NVM_DEV dev, NVM_GEO geo, NVM_ADDR list[], int len, int flags)
{
	return 0;
}

typedef struct {
	char name[CLI_CMD_LEN];
	ssize_t (*func)(NVM_DEV, NVM_GEO, NVM_ADDR[], int, int);
	int argc;
	int flags;
} NVM_CLI_BBT_CMD;

static NVM_CLI_BBT_CMD cmds[] = {
	{"get", get, 3, 0x0},
	{"set", set, -1, 0x0}
};

static int ncmds = sizeof(cmds) / sizeof(cmds[0]);

void _usage_pr(char *cli_name)
{
	printf("Usage:\n");
	printf(" %6s get dev_path ch lun\n", cli_name);
	printf(" %6s set dev_path ch lun blk [blk...]\n", cli_name);
}

int main(int argc, char **argv)
{
	char cmd_name[CLI_CMD_LEN];
	char dev_path[NVM_DEV_PATH_LEN+1];
	int ret, i, ninvalid;

	NVM_CLI_BBT_CMD *cmd = NULL;

	NVM_DEV dev;
	NVM_GEO geo;
	NVM_ADDR list[1024];
	int len;

	if (argc < 4) {
		_usage_pr(argv[0]);
		return -EINVAL;
	}
							// Get `cmd_name`
	if (strlen(argv[1]) < 1 || strlen(argv[1]) > (CLI_CMD_LEN-1)) {
		printf("Invalid cmd\n");
		_usage_pr(argv[0]);
		return -EINVAL;
	}
	memset(cmd_name, 0, sizeof(cmd_name));
	strcpy(cmd_name, argv[1]);

	for (i = 0; i < ncmds; ++i) {			// Get `cmd`
		if (strcmp(cmd_name, cmds[i].name) == 0) {
			cmd = &cmds[i];
			break;
		}
	}
	if (!cmd) {
		printf("Invalid cmd(%s)\n", cmd_name);
		_usage_pr(argv[0]);
		return -EINVAL;
	}
							// Get `dev_path`
	if (strlen(argv[2]) < 1 || strlen(argv[2]) > NVM_DEV_PATH_LEN) {
		printf("len(dev_path) > %d\n", NVM_DEV_PATH_LEN);
		return -EINVAL;
	}
	memset(dev_path, 0, sizeof(dev_path));
	strcpy(dev_path, argv[2]);

	switch(cmd->argc) {				// Get `list` and `len`
		case -1:				// ppa [ppa..]
			len = argc - 3;
			for (i = 0; i < len; ++i) {
				list[i].ppa = atol(argv[i+3]);
				list[i].g.pl = 0;
				list[i].g.blk = 0;
				list[i].g.pg = 0;
				list[i].g.sec = 0;
			}
			break;
		case 9:					// ch lun pl blk pg sec
			len = 1;
			list[0].g.ch = atoi(argv[3]);
			list[0].g.lun = atoi(argv[4]);
			list[0].g.pl = 0;
			list[0].g.blk = 0;
			list[0].g.pg = 0;
			list[0].g.sec = 0;
			break;
		default:
			printf("Invalid argc(%d) for cmd(%s)\n",
				cmd->argc, cmd_name);
			_usage_pr(argv[0]);
			return -EINVAL;
	}

	dev = nvm_dev_open(dev_path);
	if (!dev) {
		printf("Failed opening device, dev_path(%s)\n", dev_path);
		return -EINVAL;
	}
	geo = nvm_dev_attr_geo(dev);

	ninvalid = 0;
	for (i = 0; i < len; ++i) {
		int mask;

		mask = nvm_addr_check(list[i], geo);
		if (mask) {
			nvm_addr_pr(list[i]);
			printf("exceeds: ");
			nvm_bounds_pr(mask);
			ninvalid++;
		}
	}
	if (ninvalid) {
		printf("ninvalid(%d) addresses exceeds device boundaries\n",
			ninvalid);
		nvm_geo_pr(geo);
		ret = EINVAL;
	} else {
		ret = cmd->func(dev, geo, list, len, cmd->flags);
		if (ret)
			printf("Command failed\n");
	}

	nvm_dev_close(dev);

	return ret;
}
