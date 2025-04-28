GST_APP_NAME = gst-open-gate-rtsp-sender
ADB          = /usr/bin/adb
DEPLOY_DIR   = /usr/bin
ifdef ADB_ID
# remember to add a space after $(ADB_ID)
ADB_SPEC := -s $(ADB_ID) 
endif
SSH		= ssh
SCP		= scp


SOURCES = \
        src/main.cc
INCLUDES += -I ./include/
INCLUDES += -I ${SDKTARGETSYSROOT}/usr/include
INCLUDES += -I ${SDKTARGETSYSROOT}/usr/include/glib-2.0
INCLUDES += -I ${SDKTARGETSYSROOT}/usr/lib/glib-2.0/include
INCLUDES += -I ${SDKTARGETSYSROOT}/usr/include/gstreamer-1.0
INCLUDES += -I /home/andrew/QSDK/SDK/workspace/sources/qcom-gst-sample-apps-utils
TARGETS = $(foreach n,$(SOURCES),$(basename $(n)))

LLIBS    += -lgstreamer-1.0 -lgobject-2.0 -lglib-2.0

STDSET = -std=c++17

# CXX = aarch64-qcom-linux-g++

all: ${TARGETS}

.PHONY: ${TARGETS}

${TARGETS}: %:%.cc
# $(CXX) -Wall --sysroot=$(SDKTARGETSYSROOT) $(INCLUDES) $(LLIBS) $< -o $(GST_APP_NAME)
	$(CXX) $(STDSET) -Wall  $(INCLUDES) $(LLIBS) $< -o $(GST_APP_NAME)

build-debug:
# $(CXX) -Wall -g --sysroot=$(SDKTARGETSYSROOT) $(INCLUDES) $(LLIBS) $(SOURCES) -o $(GST_APP_NAME)
	$(CXX) $(CXXFLAGS) $(STDSET) -Wall -g $(INCLUDES) $(LLIBS) $(SOURCES) -o $(GST_APP_NAME)

deploy:
	$(DEPLOYER) ${ADB_ID} ${GST_APP_NAME} $(DEPLOY_DIR)
deploy-ssh:
	$(SCP) -i $(SSHKEY) ${GST_APP_NAME} root@$(IPADDR):$(DEPLOY_DIR)/$(GST_APP_NAME)

run:
	$(ADB) ${ADB_SPEC}shell 'export GST_DEBUG=2 && ${GST_APP_NAME}'
run-ssh:
	$(SSH) -i $(SSHKEY)  root@$(IPADDR) "export GST_DEBUG=2 && ${GST_APP_NAME}"

stop:
	$(ADB) ${ADB_SPEC}shell 'killall ${GST_APP_NAME}'
stop-ssh:
	$(SSH) -i $(SSHKEY)  root@$(IPADDR) "killall ${GST_APP_NAME}"

debug:
	$(ADB) $(ADB_SPEC)forward tcp:1234 tcp:1234
	$(ADB) $(ADB_SPEC)shell 'gdbserver :1234 $(DEPLOY_DIR)/$(GST_APP_NAME)'
debug-ssh:
	$(SSH) -i $(SSHKEY) -R 1235:localhost:1235 root@$(IPADDR) "exit"
	$(SSH) -i $(SSHKEY)  root@$(IPADDR) "gdbserver :1235 $(DEPLOY_DIR)/$(GST_APP_NAME)"

clean:
	rm -f ${TARGETS}


