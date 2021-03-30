nobase_include_HEADERS = \
	ipmid/api.hpp \
	ipmid/api-types.hpp \
	ipmid/sessiondef.hpp \
	ipmid/sessionhelper.hpp \
	ipmid/filter.hpp \
	ipmid/handler.hpp \
	ipmid/message.hpp \
	ipmid/message/pack.hpp \
	ipmid/message/types.hpp \
	ipmid/message/unpack.hpp \
	ipmid/api.h \
	ipmid/iana.hpp \
	ipmid/oemopenbmc.hpp \
	ipmid/oemrouter.hpp \
	ipmid/types.hpp \
	ipmid/utility.hpp \
	ipmid/utils.hpp \
	ipmid-host/cmd.hpp \
	ipmid-host/cmd-utils.hpp

# Eventually we will split <ipmid/*> and <host-ipmid/*> headers
# For now they will be the same during migration
install-data-local:
	$(MKDIR_P) $(DESTDIR)$(includedir)/host-ipmid
	$(LN_S) -f ../ipmid/iana.hpp $(DESTDIR)$(includedir)/host-ipmid/iana.hpp
	$(LN_S) -f ../ipmid/api.h $(DESTDIR)$(includedir)/host-ipmid/ipmid-api.h
	$(LN_S) -f ../ipmid/oemopenbmc.hpp $(DESTDIR)$(includedir)/host-ipmid/oemopenbmc.hpp
	$(LN_S) -f ../ipmid/oemrouter.hpp $(DESTDIR)$(includedir)/host-ipmid/oemrouter.hpp
	$(LN_S) -f ../ipmid-host/cmd.hpp $(DESTDIR)$(includedir)/host-ipmid/ipmid-host-cmd.hpp
	$(LN_S) -f ../ipmid-host/cmd-utils.hpp $(DESTDIR)$(includedir)/host-ipmid/ipmid-host-cmd-utils.hpp

