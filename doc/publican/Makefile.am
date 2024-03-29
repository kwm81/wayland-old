# Documentation is built with publican
# https://fedorahosted.org/publican/
# Publican takes docbook-style input files and compiles them to various
# output formats.
#
# How this build works:
# * the main target is Wayland, documentation ends up in $(builddir)/Wayland/
# * hand-written chapters are located in sources
#   Publican does not take a source path, so to support out-of-tree builds
#   these are copied to $(builddir)/en-US which is the actual directory
#   Publican uses.
# * ProtocolSpec.xml is generated from $(top_srcdir)/protocol/wayland.xml,
#   changed into docbook via XSLT and saved in $(builddir)/en-US/
# * ProtocolInterfaces.xml, same as above, uses a different XSLT
# * WaylandClientAPI.xml is generated from the doxygen output and saved in
#   $(builddir)/en-US
# * WaylandServerAPI.xml is generated from the doxygen output and saved in
#   $(builddir)/en-US
# * run Publican on en-US
publican_sources = \
	$(srcdir)/sources/Wayland.ent \
	$(srcdir)/sources/Wayland.xml \
	$(srcdir)/sources/Book_Info.xml \
	$(srcdir)/sources/Author_Group.xml \
	$(srcdir)/sources/Foreword.xml \
	$(srcdir)/sources/Preface.xml \
	$(srcdir)/sources/Revision_History.xml \
	$(srcdir)/sources/Introduction.xml \
	$(srcdir)/sources/Architecture.xml \
	$(srcdir)/sources/Protocol.xml \
	$(srcdir)/sources/Library.xml \
	$(srcdir)/sources/Compositors.xml \
	$(srcdir)/sources/images/icon.svg  \
	$(srcdir)/sources/images/wayland-architecture.png \
	$(srcdir)/sources/images/wayland.png  \
	$(srcdir)/sources/images/x-architecture.png

if HAVE_PUBLICAN
if HAVE_XSLTPROC
noinst_DATA = Wayland $(publican_targets)
pubdir = $(docdir)/Wayland/en-US

publican_targets = $(publican_sources:$(srcdir)/sources%=$(builddir)/en-US%) \
	en-US/ProtocolSpec.xml en-US/ProtocolInterfaces.xml \
	en-US/WaylandClientAPI.xml en-US/WaylandServerAPI.xml

# The Protocol.xml is purely generated and required before running publican
en-US/ProtocolSpec.xml: $(top_srcdir)/protocol/wayland.xml $(srcdir)/protocol-to-docbook.xsl
	$(AM_V_GEN)$(MKDIR_P) en-US/images
	$(AM_V_GEN)$(XSLTPROC) $(srcdir)/protocol-to-docbook.xsl \
		$(top_srcdir)/protocol/wayland.xml > en-US/ProtocolSpec.xml

en-US/ProtocolInterfaces.xml: $(top_srcdir)/protocol/wayland.xml $(srcdir)/protocol-interfaces-to-docbook.xsl
	$(AM_V_GEN)$(MKDIR_P) en-US/images
	$(AM_V_GEN)$(XSLTPROC) $(srcdir)/protocol-interfaces-to-docbook.xsl \
		$(top_srcdir)/protocol/wayland.xml > en-US/ProtocolInterfaces.xml

combine_xml: server client

# * we don't want wayland-{server|client}_8h.xml to avoid duplicating output methods,
#   move it out of the way first.
# * use doxygen's combine.xslt to merge the xml files into one single file
# * move wayland-<foo>_8h.xml back to its original location
server client:
	$(AM_V_GEN)mv $(top_builddir)/doc/doxygen/xml/$@/wayland-$@_8h.xml \
		$(top_builddir)/doc/doxygen/xml/
	$(AM_V_GEN)$(XSLTPROC) $(top_builddir)/doc/doxygen/xml/$@/combine.xslt \
		$(top_builddir)/doc/doxygen/xml/$@/index.xml > \
		$(top_builddir)/doc/doxygen/xml/$@/$@API.xml
	$(AM_V_GEN)mv $(top_builddir)/doc/doxygen/xml/wayland-$@_8h.xml \
		$(top_builddir)/doc/doxygen/xml/$@

# WaylandClientAPI.xml:
# merge doxygen xml files into one single file (see combine_xml), then transform the combined XML file into docbook format
en-US/WaylandClientAPI.xml: combine_xml $(top_builddir)/doc/doxygen/xml/client/index.xml $(srcdir)/doxygen-to-publican.xsl
	$(AM_V_GEN)$(XSLTPROC)  --stringparam which Client $(srcdir)/doxygen-to-publican.xsl \
		$(top_builddir)/doc/doxygen/xml/client/clientAPI.xml > en-US/WaylandClientAPI.xml

# WaylandServerAPI.xml: see WaylandClientAPI.xml
en-US/WaylandServerAPI.xml: combine_xml $(top_builddir)/doc/doxygen/xml/client/index.xml $(srcdir)/doxygen-to-publican.xsl
	$(AM_V_GEN)$(XSLTPROC) --stringparam which Server $(srcdir)/doxygen-to-publican.xsl \
		$(top_builddir)/doc/doxygen/xml/server/serverAPI.xml > en-US/WaylandServerAPI.xml

# Copy the sources source files into en-US destination
# This is required for out-of-source-tree build as publican does not allow us
# to specify the location of the source code.
$(builddir)/en-US/%: $(srcdir)/sources/% en-US/ProtocolSpec.xml en-US/ProtocolInterfaces.xml en-US/WaylandClientAPI.xml $(publican_sources)
	$(AM_V_GEN)cp -f $< $@
	$(AM_V_GEN)chmod a+w $@

# Run publican for the builddir on the generated (or copied) source
# The output formats are generated in the Wayland sub directory. Also, we need
# to use a tmp publican.cfg cause 'publican rename' modifies the original.
Wayland: $(publican_targets)
	$(AM_V_GEN)cp -f $(srcdir)/publican.cfg $(builddir)/publican-copy.cfg
	$(AM_V_GEN)$(PUBLICAN) rename --name Wayland \
		--version "$(WAYLAND_VERSION_MAJOR).$(WAYLAND_VERSION_MINOR)" \
		--config $(builddir)/publican-copy.cfg
	$(AM_V_GEN)$(PUBLICAN) build --quiet --langs en-US --pdftool fop --formats html,pdf \
		--config $(builddir)/publican-copy.cfg
	@touch Wayland

CLEANFILES = en-US/ProtocolSpec.xml en-US/ProtocolInterfaces.xml en-US/WaylandClientAPI.xml $(publican_targets)

clean-local:
	$(AM_V_at)rm -fr $(builddir)/en-US
	$(AM_V_at)rm -fr $(builddir)/Wayland
	$(AM_V_at)rm -fr $(builddir)/publican-copy.cfg

install-data-local:
	test -z "$(pubdir)/html/Common_Content/css" || $(mkdir_p) "$(DESTDIR)$(pubdir)/html/Common_Content/css"
	test -z "$(pubdir)/html/Common_Content/images" || $(mkdir_p) "$(DESTDIR)$(pubdir)/html/Common_Content/images"
	test -z "$(pubdir)/html/images" || $(mkdir_p) "$(DESTDIR)$(pubdir)/html/images"
	test -z "$(pubdir)/html-pdf/Common_Content/css" || $(mkdir_p) "$(DESTDIR)$(pubdir)/html-pdf/Common_Content/css"
	test -z "$(pubdir)/html-pdf/Common_Content/images" || $(mkdir_p) "$(DESTDIR)$(pubdir)/html-pdf/Common_Content/images"
	test -z "$(pubdir)/html-pdf/images" || $(mkdir_p) "$(DESTDIR)$(pubdir)/html-pdf/images"
	test -z "$(pubdir)/pdf" || $(mkdir_p) "$(DESTDIR)$(pubdir)/pdf"
	test -z "$(pubdir)/xml/Common_Content/css" || $(mkdir_p) "$(DESTDIR)$(pubdir)/xml/Common_Content/css"
	test -z "$(pubdir)/xml/Common_Content/images" || $(mkdir_p) "$(DESTDIR)$(pubdir)/xml/Common_Content/images"
	test -z "$(pubdir)/xml/images" || $(mkdir_p) "$(DESTDIR)$(pubdir)/xml/images"
	list=`find $(builddir)/Wayland/en-US -type f -not -path '$(builddir)/Wayland/en-US/xml_tmp*'`; \
	for p in $$list; do \
	  echo " $(INSTALL_DATA) '$$p' '$(DESTDIR)$(docdir)/$$p'"; \
	  $(INSTALL_DATA) "$$p" "$(DESTDIR)$(docdir)/$$p"; \
	done;

uninstall-local:
	@if test -n $(DESTDIR)$(docdir); then \
	  if test -d $(DESTDIR)$(docdir); then \
	    echo " rm -fr $(DESTDIR)$(docdir)/Wayland;"; \
	    rm -fr $(DESTDIR)$(docdir)/Wayland; \
	  fi; \
	fi;

endif
endif

EXTRA_DIST = $(publican_sources) publican.cfg protocol-to-docbook.xsl protocol-interfaces-to-docbook.xsl doxygen-to-publican.xsl
