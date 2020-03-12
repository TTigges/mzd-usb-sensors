#
# Recursively call make for all targets
#

SUBDIRS= cmu local

all:
	for dir in $(SUBDIRS); do   \
	  echo;                     \
	  echo "##### $$dir #####"; \
	  $(MAKE) -C $$dir;         \
	done

clean:
	for dir in $(SUBDIRS); do   \
	  echo;                     \
	  echo "##### $$dir #####"; \
	  $(MAKE) -C $$dir clean;   \
	done
