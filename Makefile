# Name:							Lukáš Drahník
# Project: 					ISA: Měření ztrátovosti a RTT (Matěj Grégr)
#	Date:							30.9.2017
# Email:						<xdrahn00@stud.fit.vutbr.cz>, <ldrahnik@gmail.com>

PROJECT_NAME     		= testovac
PROJECT_MAN_PAGE		= testovac.1
PROJECT_LICENSE	 		= LICENSE
PROJECT_DOC				= manual.pdf
PROJECT_DOC_FOLDER		= doc/
PROJECT_SPEC		 	= testovac.spec
PROJECT_SOURCES_CODE	= $(shell find src/ -name *.cpp)
PROJECT_SOURCES_HEADERS = src/*.h
PROJECT_SOURCES			= $(PROJECT_SOURCES_CODE) $(PROJECT_SOURCES_HEADERS)
PROJECT_OBJECTS			= $(PROJECT_SOURCES:%.cpp=%.o) 
PROJECT_OBJECT_FILES	= src/*.o
CC              		= g++
CFLAGS 					= -Wall -pedantic -Wextra -pthread

###########################################

all:			$(PROJECT_NAME)

$(PROJECT_NAME): $(PROJECT_OBJECTS)
		$(CC) $(CFLAGS) $(PROJECT_SOURCES) -o $@

clean:
	rm -rf *~ $(PROJECT_OBJECT_FILES) $(PROJECT_NAME)
	cd doc && make clean

rebuild:	clean all

############################################

BUILD_ROOT		  =
VERSION					=
INSTALL_DIR			= $(BUILD_ROOT)/usr/lib/$(PROJECT_NAME)
INSTALL_SOURCES = $(PROJECT_NAME)

MAN_DIR					= $(BUILD_ROOT)/usr/share/man/man1
BIN_DIR					= $(BUILD_ROOT)/usr/bin

SHARE_DIR				= $(BUILD_ROOT)/usr/share
DOC_DIR					= $(SHARE_DIR)/doc/$(PROJECT_NAME)
LICENSES_DIR		= $(SHARE_DIR)/licenses/$(PROJECT_NAME)

install:
	mkdir -p $(INSTALL_DIR)
	mkdir -p $(BIN_DIR)
	mkdir -p $(MAN_DIR)
	mkdir -p $(DOC_DIR)
	mkdir -p $(LICENSES_DIR)
	install -m 0644 $(INSTALL_SOURCES) $(INSTALL_DIR)
	install -m 0644 $(PROJECT_LICENSE) $(LICENSES_DIR)
	install -m 0644 $(PROJECT_DOC) $(DOC_DIR)
	cd $(BUILD_ROOT) && sudo ln -sf /usr/lib/$(PROJECT_NAME)/$(PROJECT_NAME) $(BUILD_ROOT)/usr/bin/$(PROJECT_NAME)
	sudo chmod 0755 $(INSTALL_DIR)/$(PROJECT_NAME)
	install -m 0644 $(PROJECT_MAN_PAGE) $(MAN_DIR)

############################################

LOGIN = xdrahn00
FILES = Makefile $(PROJECT_MAN_PAGE) $(PROJECT_SOURCES) $(PROJECT_LICENSE) $(PROJECT_SPEC) # exceptions are archive files placed in subfolders

tar:
	tar -cvzf $(LOGIN).tar $(FILES) -C $(PROJECT_DOC_FOLDER) $(PROJECT_DOC)

untar:
	rm -rf $(LOGIN) && mkdir -p $(LOGIN) && tar -xvzf $(LOGIN).tar -C $(LOGIN)

rmtar:
	rm -f $(LOGIN).tar

############################################

tex:
	cd doc && make && make manual.ps && make manual.pdf
