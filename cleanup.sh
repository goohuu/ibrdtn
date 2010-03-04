#!/bin/bash
#
make clean && make -C ibrcommon
sudo make uninstall
sudo make -C ibrcommon uninstall

