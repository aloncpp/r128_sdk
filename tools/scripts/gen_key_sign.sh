#!/bin/bash

SH_NAME=`readlink -f $0`

ECC_KEY_DIR=`dirname $SH_NAME`/ecc_keys
RSA_KEY_DIR=`dirname $SH_NAME`/rsa_keys

PRIKEY_ECC=private_ecc.pem
PUBKEY_ECC=public_ecc.pem
PUBKEY_ECC_DER=public_ecc.der
GENPUBKEY_ECC=key_ecc.bin
ROTPK_ECC=rotpk_ecc.bin

PRIKEY_RSA=private_rsa.pem
PUBKEY_RSA=public_rsa.pem
PUBKEY_RSA_DER=public_rsa.der
GENPUBKEY_RSA=key_rsa.bin
ROTPK_RSA=rotpk_rsa.bin

function gen_ecc_key()
{
	if [ "x$1" != "x" ]; then
		ECC_KEY_DIR=$1
	fi

	# remove old keys
	rm -rf $ECC_KEY_DIR
	mkdir -p $ECC_KEY_DIR

	# generate prime256v1 private and public key
	openssl ecparam -name prime256v1 -genkey -out $ECC_KEY_DIR/$PRIKEY_ECC
	openssl ec -in $ECC_KEY_DIR/$PRIKEY_ECC -pubout -out $ECC_KEY_DIR/$PUBKEY_ECC  > /dev/null 2>&1
	openssl ec -inform PEM -in $ECC_KEY_DIR/$PUBKEY_ECC -pubin -outform DER -out $ECC_KEY_DIR/$PUBKEY_ECC_DER  > /dev/null 2>&1

	# generate ecc key bin (x + y)
	dd if=$ECC_KEY_DIR/$PUBKEY_ECC_DER of=$ECC_KEY_DIR/$GENPUBKEY_ECC bs=1 count=64 skip=27 > /dev/null 2>&1

	# generate rotpk ecc bin
	openssl dgst -sha256 -binary -out $ECC_KEY_DIR/$ROTPK_ECC $ECC_KEY_DIR/$GENPUBKEY_ECC

	echo "Successfully generate ecc keys to path: $ECC_KEY_DIR"
}

function gen_ecc_rotpk()
{
	ECC_KEY_DIR=$1

	if [ ! -f $ECC_KEY_DIR/$PRIKEY_ECC ]; then
		echo "Error: no private key file: $ECC_KEY_DIR/$PRIKEY_ECC"
		exit 1
	fi

	openssl ec -in $ECC_KEY_DIR/$PRIKEY_ECC -pubout -out $ECC_KEY_DIR/$PUBKEY_ECC  > /dev/null 2>&1
	openssl ec -inform PEM -in $ECC_KEY_DIR/$PUBKEY_ECC -pubin -outform DER -out $ECC_KEY_DIR/$PUBKEY_ECC_DER  > /dev/null 2>&1

	# generate ecc key bin (x + y)
	dd if=$ECC_KEY_DIR/$PUBKEY_ECC_DER of=$ECC_KEY_DIR/$GENPUBKEY_ECC bs=1 count=64 skip=27 > /dev/null 2>&1

	# generate rotpk ecc bin
	openssl dgst -sha256 -binary -out $ECC_KEY_DIR/$ROTPK_ECC $ECC_KEY_DIR/$GENPUBKEY_ECC

	echo "Successfully generate ecc rotpk to path: $ECC_KEY_DIR"
}

function gen_ecc_sign()
{
#	local prefix=${1%.*}
#	local ORIGSIGN_ECC=${prefix}_orig_ecc_sign.bin
#	local SIGN_ECC=${prefix}_ecc_sign.bin
#	local TEMPSIGN_ECC=${prefix}_temp.txt

	local ORIGSIGN_ECC=${1}.orig_ecc_sign
	local SIGN_ECC=${1}.ecc_sign
	local TEMPSIGN_ECC=${1}.temp_sign

	if [ "x$2" != "x" ]; then
		ECC_KEY_DIR=$2
	fi

	# signature
	openssl dgst -sign $ECC_KEY_DIR/$PRIKEY_ECC -sha256 -out $ORIGSIGN_ECC $1

	# verify
	openssl dgst -verify $ECC_KEY_DIR/$PUBKEY_ECC -sha256 -signature $ORIGSIGN_ECC $1 > /dev/null
	if [ $? -ne 0 ]; then
		echo "verify error!"
		rm -rf $ORIGSIGN_ECC
		exit 1
	fi

	# extract signature r and s
	local offset=0
	local length=0

	dd if=$ORIGSIGN_ECC of=$TEMPSIGN_ECC bs=1 count=1 skip=3 > /dev/null 2>&1
	length=`cat $TEMPSIGN_ECC`;length=`printf "%x" "'$length"`
	if [[ "$length" == "21" ]]; then
			dd if=$ORIGSIGN_ECC of=$SIGN_ECC bs=1 count=32 skip=5 > /dev/null 2>&1
			offset=37
	else
			dd if=$ORIGSIGN_ECC of=$SIGN_ECC bs=1 count=32 skip=4 > /dev/null 2>&1
			offset=36
	fi

	dd if=$ORIGSIGN_ECC of=$TEMPSIGN_ECC bs=1 count=1 skip=$(($offset+1)) > /dev/null 2>&1
	length=`cat $TEMPSIGN_ECC`;length=`printf "%x" "'$length"`
	if [[ "$length" == "21" ]]; then
			dd if=$ORIGSIGN_ECC of=$SIGN_ECC bs=1 count=32 skip=$(($offset+3)) seek=32 > /dev/null 2>&1
	else
			dd if=$ORIGSIGN_ECC of=$SIGN_ECC bs=1 count=32 skip=$(($offset+2)) seek=32 > /dev/null 2>&1
	fi

	#rm -f $ORIGSIGN_ECC
	rm -f $TEMPSIGN_ECC

	echo "Successfully generate signature file: $SIGN_ECC"
}

function gen_rsa_key()
{
	if [ "x$1" != "x" ]; then
		RSA_KEY_DIR=$1
	fi

	# remove old keys
	rm -rf $RSA_KEY_DIR
	mkdir -p $RSA_KEY_DIR

	# generate rsa2048 private and public key
	openssl genrsa -out $RSA_KEY_DIR/$PRIKEY_RSA 2048 > /dev/null 2>&1
	openssl rsa -in $RSA_KEY_DIR/$PRIKEY_RSA -pubout -out $RSA_KEY_DIR/$PUBKEY_RSA > /dev/null 2>&1
	#openssl rsa -in $RSA_KEY_DIR/$PRIKEY_RSA -pubout -out $RSA_KEY_DIR/$RSA_KEY_DIR/$PUBKEY_RSA_DER -outform dcr

	# generate rsa_key.bin (n + e + 0x91 padding)
	openssl rsa -modulus -pubin -in $RSA_KEY_DIR/$PUBKEY_RSA -noout | awk -F = '{print $2}' | xxd -r -ps > $RSA_KEY_DIR/$GENPUBKEY_RSA
	openssl rsa -pubin -in $RSA_KEY_DIR/$PUBKEY_RSA -text -noout | grep 65537 > /dev/null
	if [ $? -eq 0 ]; then
		echo 010001 | xxd -r -ps >> $RSA_KEY_DIR/$GENPUBKEY_RSA
	else
		echo "Error: RSA Exponent is not 65537"
		rm $RSA_KEY_DIR/$PRIKEY_RSA $RSA_KEY_DIR/$PUBKEY_RSA $RSA_KEY_DIR/$RSA_KEY_DIR/$PUBKEY_RSA_DER $RSA_KEY_DIR/$GENPUBKEY_RSA $RSA_KEY_DIR/$ROTPK_RSA
		exit 1
	fi
	#dd if=/dev/zero bs=1 count=253 | tr "\000" "\221" >> $RSA_KEY_DIR/$GENPUBKEY_RSA
	dd if=/dev/zero of=$RSA_KEY_DIR/temp.bin bs=1 count=253	> /dev/null 2>&1
	cat $RSA_KEY_DIR/temp.bin | tr "\000" "\221" >> $RSA_KEY_DIR/$GENPUBKEY_RSA
	rm -rf $RSA_KEY_DIR/temp.bin

	# generate rotpk_rsa.bin
	openssl dgst -sha256 -binary -out $RSA_KEY_DIR/$ROTPK_RSA $RSA_KEY_DIR/$GENPUBKEY_RSA

	echo "Successfully generate rsa keys to path: $RSA_KEY_DIR"
}

function gen_rsa_rotpk()
{

	RSA_KEY_DIR=$1

	if [ ! -f $RSA_KEY_DIR/$PRIKEY_RSA ]; then
		echo "Error: no private key file: $RSA_KEY_DIR/$PRIKEY_RSA"
		exit 1
	fi

	openssl rsa -in $RSA_KEY_DIR/$PRIKEY_RSA -pubout -out $RSA_KEY_DIR/$PUBKEY_RSA > /dev/null 2>&1
	#openssl rsa -in $RSA_KEY_DIR/$PRIKEY_RSA -pubout -out $RSA_KEY_DIR/$RSA_KEY_DIR/$PUBKEY_RSA_DER -outform dcr

	# generate rsa_key.bin (n + e + 0x91 padding)
	openssl rsa -modulus -pubin -in $RSA_KEY_DIR/$PUBKEY_RSA -noout | awk -F = '{print $2}' | xxd -r -ps > $RSA_KEY_DIR/$GENPUBKEY_RSA
	openssl rsa -pubin -in $RSA_KEY_DIR/$PUBKEY_RSA -text -noout | grep 65537 > /dev/null
	if [ $? -eq 0 ]; then
		echo 010001 | xxd -r -ps >> $RSA_KEY_DIR/$GENPUBKEY_RSA
	else
		echo "Error: RSA Exponent is not 65537"
		rm $RSA_KEY_DIR/$PRIKEY_RSA $RSA_KEY_DIR/$PUBKEY_RSA $RSA_KEY_DIR/$RSA_KEY_DIR/$PUBKEY_RSA_DER $RSA_KEY_DIR/$GENPUBKEY_RSA $RSA_KEY_DIR/$ROTPK_RSA
		exit 1
	fi
	#dd if=/dev/zero bs=1 count=253 | tr "\000" "\221" >> $RSA_KEY_DIR/$GENPUBKEY_RSA
	dd if=/dev/zero of=$RSA_KEY_DIR/temp.bin bs=1 count=253	> /dev/null 2>&1
	cat $RSA_KEY_DIR/temp.bin | tr "\000" "\221" >> $RSA_KEY_DIR/$GENPUBKEY_RSA
	rm -rf $RSA_KEY_DIR/temp.bin

	# generate rotpk_rsa.bin
	openssl dgst -sha256 -binary -out $RSA_KEY_DIR/$ROTPK_RSA $RSA_KEY_DIR/$GENPUBKEY_RSA

	echo "Successfully generate rsa rotpk to path: $RSA_KEY_DIR"
}

function gen_rsa_sign()
{
#	local prefix=${1%.*}
#	local SIGN_RSA=${prefix}_rsa_sign.bin
	local SIGN_RSA=${1}.rsa_sign

	if [ "x$2" != "x" ]; then
		RSA_KEY_DIR=$2
	fi

	# signature
	openssl dgst -sign $RSA_KEY_DIR/$PRIKEY_RSA -sha256 -out $SIGN_RSA $1

	# verify
	openssl dgst -verify $RSA_KEY_DIR/$PUBKEY_RSA -sha256 -signature $SIGN_RSA $1 > /dev/null
	if [ $? -ne 0 ]; then
		echo "verify error!"
		rm -rf $SIGN_RSA
		exit 1
	fi

	####### comment another method
	if false; then
		#######  do not use openssl dgst, refer https://stackoverflow.com/questions/9951559/difference-between-openssl-rsautl-and-dgst
		local IMAGE_SHA256=${1}.sha256

		openssl dgst -sha256 -binary -out $IMAGE_SHA256 $1

		openssl rsautl -sign -in $IMAGE_SHA256 -inkey scripts/test_rsa/private_rsa.pem -out $SIGN_RSA
		if [ $? -ne 0 ]; then
			echo "generate rsa signature error!"
			rm -rf $SIGN_RSA $IMAGE_SHA256
			exit 1
		fi

		openssl rsautl -verify -in $SIGN_RSA -pubin -inkey scripts/test_rsa/public_rsa.pem -out ${IMAGE_SHA256}_v
		if [ $? -ne 0 ]; then
			echo "verify rsa signature error!"
			rm -rf $SIGN_RSA $IMAGE_SHA256 ${IMAGE_SHA256}_v
			exit 1
		fi

		diff $IMAGE_SHA256 ${IMAGE_SHA256}_v > /dev/null
		if [ $? -ne 0 ]; then
			echo "verify rsa signature failed!"
			rm -rf $SIGN_RSA $IMAGE_SHA256 ${IMAGE_SHA256}_v
			exit 1
		fi

		rm -rf $IMAGE_SHA256 ${IMAGE_SHA256}_v
	fi

	echo "Successfully generate signature file: $SIGN_RSA"
}

function usage()
{
		echo "Error params!"
		echo ""
		echo "Usage1:  $0 key <ecc|rsa> [key_path]"
		echo "    generate key to [key_path], default [key_path]"
		echo "    is ${0%/*}/ecc_keys or ${0%/*}/rsa_keys"
		echo ""
		echo "Usage2:  $0 rotpk <ecc|rsa> [key_path]"
		echo "    generate rotpk based on [key_path]"
		echo ""
		echo "Usage3:  $0 sign <ecc|rsa> <file> [key_path]"
		echo "    sign <file> with key from [key_path], default"
		echo "    [key_path] is ${0%/*}/ecc_keys or ${0%/*}/rsa_keys"
}

case "$1" in
	key)
		if [ $# -ne 2 -a $# -ne 3 ]; then
			usage
			exit
		fi

		if [ "x$2" == "xecc" ]; then
			gen_ecc_key $3
		elif [ "x$2" == "xrsa" ]; then
			gen_rsa_key $3
		else
			usage
		fi
		;;
	rotpk)
		if [ $# -ne 3 ]; then
			usage
			exit
		fi

		if [ "x$2" == "xecc" ]; then
			gen_ecc_rotpk $3
		elif [ "x$2" == "xrsa" ]; then
			gen_rsa_rotpk $3
		else
			usage
		fi
		;;
	sign)
		if [ $# -ne 3 -a $# -ne 4 ]; then
			usage
			exit
		fi

		if [ "x$2" == "xecc" ]; then
			if [ $# -eq 3 -a ! -f $ECC_KEY_DIR/$PRIKEY_ECC ]; then
				echo "Error: no $PRIKEY_ECC keys in $ECC_KEY_DIR, please generate first."
				exit
			fi
			if [ $# -eq 4 -a ! -f $4/$PRIKEY_ECC ]; then
				echo "Error: no $PRIKEY_ECC keys in $4, please generate first."
				exit
			fi
			gen_ecc_sign $3 $4
		elif [ "x$2" == "xrsa" ]; then
			if [ $# -eq 3 -a ! -f $RSA_KEY_DIR/$PRIKEY_RSA ]; then
				echo "Error: no $PRIKEY_RSA keys in $ECC_KEY_DIR, please generate first."
				exit
			fi
			if [ $# -eq 4 -a ! -f $4/$PRIKEY_RSA ]; then
				echo "Error: no $PRIKEY_RSA keys in $4, please generate first."
				exit
			fi
			gen_rsa_sign $3 $4
		else
			usage
		fi
		;;
	*)
		usage
		;;
esac;
