LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

strongswan_CHARON_PLUGINS := android-log openssl fips-prf random nonce pubkey \
	pkcs1 pkcs8 pem xcbc hmac socket-default \
	eap-identity eap-mschapv2 eap-md5 eap-gtc

strongswan_PLUGINS := $(strongswan_CHARON_PLUGINS)

strongswan_PATH := $(LOCAL_PATH)/../../../../../strongswan

# includes
libvstr_PATH := $(LOCAL_PATH)/vstr/include
openssl_PATH := $(LOCAL_PATH)/openssl/include

include $(strongswan_PATH)/Android.common.mk

# CFLAGS (partially from a configure run using droid-gcc)
strongswan_CFLAGS := \
	-Wall \
	-Wextra \
	-Wno-format \
	-Wno-pointer-sign \
	-Wno-pointer-arith \
	-Wno-sign-compare \
	-Wno-strict-aliasing \
	-Wno-unused-parameter \
	-DHAVE___BOOL \
	-DHAVE_STDBOOL_H \
	-DHAVE_ALLOCA_H \
	-DHAVE_ALLOCA \
	-DHAVE_CLOCK_GETTIME \
	-DHAVE_PTHREAD_COND_TIMEDWAIT_MONOTONIC \
	-DHAVE_PRCTL \
	-DHAVE_LINUX_UDP_H \
	-DHAVE_STRUCT_SADB_X_POLICY_SADB_X_POLICY_PRIORITY \
	-DHAVE_IN6_PKTINFO \
	-DHAVE_IPSEC_MODE_BEET \
	-DHAVE_IPSEC_DIR_FWD \
	-DHAVE_NETINET_IP6_H \
	-DOPENSSL_NO_ENGINE \
	-DOPENSSL_NO_CMS \
	-DCONFIG_H_INCLUDED \
	-DCAPABILITIES \
	-DCAPABILITIES_NATIVE \
	-DMONOLITHIC \
	-DUSE_IKEV1 \
	-DUSE_IKEV2 \
	-DUSE_VSTR \
	-DDEBUG \
	-DCHARON_UDP_PORT=0 \
	-DCHARON_NATT_PORT=0 \
	-DVERSION=\"$(strongswan_VERSION)\" \
	-DDEV_RANDOM=\"/dev/random\" \
	-DDEV_URANDOM=\"/dev/urandom\"

# only for Android 2.0+
strongswan_CFLAGS += \
	-DHAVE_IN6ADDR_ANY

include $(addprefix $(LOCAL_PATH)/,$(addsuffix /Android.mk, \
		vstr \
		openssl \
		libandroidbridge \
))

include $(addprefix $(strongswan_PATH)/,$(addsuffix /Android.mk, \
		src/libipsec \
		src/libcharon \
		src/libhydra \
		src/libstrongswan \
))
