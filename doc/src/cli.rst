.. _sec-cli:

========================
 Command-Line Interface
========================

The command-line interface (CLI) makes most of the :ref:`sec-c-api`
functionality available from the command-line. The purpose of the CLI are
manyfold:

 * The CLI source code serve as example code of using the :ref:`sec-c-api`
 * The CLI itself provide tools for experimentation and debugging
 * A supplement to unit-testing the :ref:`sec-c-api`

.. note:: This CLI is not meant to be tool for conditioning or initialitization of Open-Channel SSDs, for such purposes see `lvnm-tools <https://github.com/OpenChannelSSD/lnvm-tools>`_ and `nvme-cli <https://github.com/linux-nvme/nvme-cli>`_

.. toctree::

nvm_dev
=======

.. code-block:: bash

  Usage:
  nvm_dev /dev/dev_name
  

nvm_addr
========

.. code-block:: bash

  Usage:
   nvm_addr  erase dev_path ppa [ppa...]
   nvm_addr  write dev_path ppa [ppa...]
   nvm_addr write_m dev_path ppa [ppa...]
   nvm_addr   read dev_path ppa [ppa...]
   nvm_addr read_m dev_path ppa [ppa...]
   nvm_addr  fmt_p dev_path ppa [ppa...]
   nvm_addr  fmt_g dev_path ch lun pl blk pg sec
   nvm_addr mark_f dev_path ppa [ppa...]
   nvm_addr mark_b dev_path ppa [ppa...]
   nvm_addr mark_g dev_path ppa [ppa...]
  

nvm_vblk
========

.. code-block:: bash

  Usage:
   nvm_vblk    get dev_path ch lun
   nvm_vblk    put dev_path ch lun blk
   nvm_vblk   read dev_path ch lun blk
   nvm_vblk  write dev_path ch lun blk
   nvm_vblk  erase dev_path ch lun blk
   nvm_vblk  pread dev_path ch lun blk pg
   nvm_vblk pwrite dev_path ch lun blk pg
  OR using PPA (parts as above are extracted from address):
   nvm_vblk    get dev_path ppa
   nvm_vblk    put dev_path ppa
   nvm_vblk   read dev_path ppa
   nvm_vblk  write dev_path ppa
   nvm_vblk  erase dev_path ppa
   nvm_vblk  pread dev_path ppa
   nvm_vblk pwrite dev_path ppa
  

nvm_sblk
========

.. code-block:: bash

  Usage:
   nvm_sblk  erase dev_path ch_bgn ch_end lun_bgn lun_end blk
   nvm_sblk  write dev_path ch_bgn ch_end lun_bgn lun_end blk
   nvm_sblk    pad dev_path ch_bgn ch_end lun_bgn lun_end blk
   nvm_sblk   read dev_path ch_bgn ch_end lun_bgn lun_end blk
  
