DEFINES+=PROJECT_CONF_H=\"project-conf.h\"

HAVE_BANKING=1
UIP_CONF_IPV6=1
UIP_CONF_RPL=1
PROJECT_SOURCEFILES += settings.c package.c transmission.c messages.c sendout.c interface.c util.c

CONTIKI_PROJECT = ems

all: $(CONTIKI_PROJECT) 

CONTIKI = ../../..

include $(CONTIKI)/Makefile.include
