#!/bin/bash -e
# Example: Specify CAN ID to select parameter files
# e.g. CAN ID = 05200000 [DiscoveryF4 used in demo Control Panel]
# ./mm 05200000
# 'CANID_UNIT_22','05200000','UNIT_22',1,1,'U8','GEVCUr: gevcu replacement: DiscoveyF4 Control Panel'

#rm build/GEVCUr.elf
export FLOAT_TYPE=hard

# Parameters for CAN ID specified by $1

# Processor ADC parameters
if [ -r params/$1-adc_idx_v_struct.c ]; then
	export ADC_PARAM=$1-adc_idx_v_struct.c
else
	echo params/$1-adc_idx_v_struct.c does not exist
	exit 1;
fi	

# DMOC #1 parameters
if [ -r params/$1-dm1_idx_v_struct.c ]; then
	export DM1_PARAM=$1-dm1_idx_v_struct.c
else
	echo params/$1-dm1_idx_v_struct.c does not exist
	exit 2;
fi	

echo "################ CAN ID  ################"
echo $1
echo "################ PARAMETER FILES ################"
echo $ADC_PARAM : $DM1_PARAM

export I_AM_CANID=0x$1
echo I_AM_CANID
make clean
./script-all GEVCUr $1
