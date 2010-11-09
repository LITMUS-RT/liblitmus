# Generic dependency resolution. Part of liblitmus so that we don't have to
#  carry it around in every project using liblitmus.

obj-all = ${sort ${foreach target,${all},${obj-${target}}}}

# rule to generate dependency files
%.d: %.c
	@set -e; rm -f $@; \
		$(CC) -MM $(CPPFLAGS) $< > $@.$$$$; \
		sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
		rm -f $@.$$$$

ifeq ($(MAKECMDGOALS),)
MAKECMDGOALS += all
endif

ifneq ($(filter-out clean,$(MAKECMDGOALS)),)

# Pull in dependencies.
-include ${obj-all:.o=.d}

endif
