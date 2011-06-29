AC_INIT([gmpc-plugins], [0.20.0], [qball@sarine.nl])
AC_CONFIG_AUX_DIR([.])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE
AC_PROG_CC


##
# AWN 
##
AC_ARG_ENABLE(awn,
         [  --enable-awn],
         [ case "${enableval}" in
         yes)    enable_awn=yes;;
          no)    enable_awn=no;;
           *)    AC_MSG_ERROR([bad value ${enableval} for --enable-awn]);;
          esac],
         [enable_awn=yes])
 
AM_CONDITIONAL([AWN_OPT],[test x"$enable_awn" = xyes])
if test x${enable_awn} = xyes; then
    AC_CONFIG_SUBDIRS([gmpc-awn])
fi
##
# Last FM
##
AC_ARG_ENABLE(lastfm,
         [  --enable-lastfm],
         [ case "${enableval}" in
         yes)    enable_lastfm=yes;;
          no)    enable_lastfm=no;;
           *)    AC_MSG_ERROR([bad value ${enableval} for --enable-lastfm]);;
          esac],
         [enable_lastfm=yes])
 
AM_CONDITIONAL([LAST_FM_OPT],[test x"$enable_lastfm" = xyes])
if test x${enable_lastfm} = xyes; then
    AC_CONFIG_SUBDIRS([gmpc-last.fm])
fi

##
# Cover amazon
##
#AC_ARG_ENABLE(coveramazon,
#         [  --enable-coveramazon],
#         [ case "${enableval}" in
#       yes)    enable_coveramazon=yes;;
#       no)    enable_coveramazon=no;;
#           *)    AC_MSG_ERROR([bad value ${enableval} for --enable-coveramazon]);;
#          esac],
#         [enable_coveramazon=yes])
enable_coveramazon=una
AM_CONDITIONAL([COVERAMAZON_OPT], [test "$enable_coveramazon" = yes])
if test x${enable_coveramazon} = xyes; then
    AC_CONFIG_SUBDIRS([gmpc-coveramazon])
fi


##
# lyricwiki
# 
##
AC_ARG_ENABLE(lyricwiki,
         [  --enable-lyricwiki],
         [ case "${enableval}" in
         yes)    enable_lyricwiki=yes;;
          no)    enable_lyricwiki=no;;
           *)    AC_MSG_ERROR([bad value ${enableval} for --enable-lyricwiki]);;
          esac],
         [enable_lyricwiki=yes])
#enable_lyricwiki=una
AM_CONDITIONAL([LYRICWIKI_OPT], [test "$enable_lyricwiki" = yes])
if test x${enable_lyricwiki} = xyes; then
    AC_CONFIG_SUBDIRS([gmpc-lyricwiki])
fi

##
# mdcover
##
AC_ARG_ENABLE(mdcover,
         [  --enable-mdcover],
         [ case "${enableval}" in
         yes)    enable_mdcover=yes;;
          no)    enable_mdcover=no;;
           *)    AC_MSG_ERROR([bad value ${enableval} for --enable-mdcover]);;
          esac],
         [enable_mdcover=yes])
 

AM_CONDITIONAL([MDCOVER_OPT], [test "$enable_mdcover" = yes])
if test x${enable_mdcover} = xyes; then
    AC_CONFIG_SUBDIRS([gmpc-mdcover])
fi

#
# * Lyrics
# 
AC_ARG_ENABLE(lyrics,
         [  --enable-lyrics],
         [ case "${enableval}" in
         yes)    enable_lyrics=yes;;
          no)    enable_lyrics=no;;
           *)    AC_MSG_ERROR([bad value ${enableval} for --enable-lyrics]);;
          esac],
         [enable_lyrics=yes])
 

AM_CONDITIONAL([LYRICS_OPT], [test "$enable_lyrics" = yes])
if test x${enable_lyrics} = xyes; then
    AC_CONFIG_SUBDIRS([gmpc-lyrics])
fi

#
# * Extra Playlist 
# 
AC_ARG_ENABLE(extraplaylist,
         [  --enable-extraplaylist],
         [ case "${enableval}" in
         yes)    enable_extraplaylist=yes;;
          no)    enable_extraplaylist=no;;
           *)    AC_MSG_ERROR([bad value ${enableval} for --enable-extraplaylist]);;
          esac],
         [enable_extraplaylist=yes])
 

AM_CONDITIONAL([EXTRAPLAYLIST_OPT], [test "$enable_extraplaylist" = yes])
if test x${enable_extraplaylist} = xyes; then
    AC_CONFIG_SUBDIRS([gmpc-extraplaylist])
fi

#
# * Avahi 
# 
AC_ARG_ENABLE(avahi,
         [  --enable-avahi],
         [ case "${enableval}" in
         yes)    enable_avahi=yes;;
          no)    enable_avahi=no;;
           *)    AC_MSG_ERROR([bad value ${enableval} for --enable-avahi]);;
          esac],
         [enable_avahi=yes])
 

AM_CONDITIONAL([AVAHI_OPT], [test "$enable_avahi" = yes])
if test x${enable_avahi} = xyes; then
    AC_CONFIG_SUBDIRS([gmpc-avahi])
fi  

#
# * Libnotify 
# 
AC_ARG_ENABLE(libnotify,
         [  --enable-libnotify],
         [ case "${enableval}" in
         yes)    enable_libnotify=yes;;
          no)    enable_libnotify=no;;
           *)    AC_MSG_ERROR([bad value ${enableval} for --enable-libnotify]);;
          esac],
         [enable_libnotify=yes])
 

AM_CONDITIONAL([LIBNOTIFY_OPT], [test "$enable_libnotify" = yes])
if test x${enable_libnotify} = xyes; then
    AC_CONFIG_SUBDIRS([gmpc-libnotify])
fi

#
# * alarm 
# 
AC_ARG_ENABLE(alarm,
         [  --enable-alarm],
         [ case "${enableval}" in
         yes)    enable_alarm=yes;;
          no)    enable_alarm=no;;
           *)    AC_MSG_ERROR([bad value ${enableval} for --enable-alarm]);;
          esac],
         [enable_alarm=yes])
 

AM_CONDITIONAL([ALARM_OPT], [test "$enable_alarm" = yes])
if test x${enable_alarm} = xyes; then
    AC_CONFIG_SUBDIRS([gmpc-alarm])
fi

#
# jamendo 
# 
AC_ARG_ENABLE(jamendo,
         [  --enable-jamendo],
         [ case "${enableval}" in
         yes)    enable_jamendo=yes;;
          no)    enable_jamendo=no;;
           *)    AC_MSG_ERROR([bad value ${enableval} for --enable-jamendo]);;
          esac],
         [enable_jamendo=yes])
 

AM_CONDITIONAL([JAMENDO_OPT], [test "$enable_jamendo" = yes])
if test x${enable_jamendo} = xyes; then
    AC_CONFIG_SUBDIRS([gmpc-jamendo])
fi

#
# lirc 
# 
AC_ARG_ENABLE(lirc,
         [  --enable-lirc],
         [ case "${enableval}" in
         yes)    enable_lirc=yes;;
          no)    enable_lirc=no;;
           *)    AC_MSG_ERROR([bad value ${enableval} for --enable-lirc]);;
          esac],
         [enable_lirc=yes])
 

AM_CONDITIONAL([LIRC_OPT], [test "$enable_lirc" = yes])
if test x${enable_lirc} = xyes; then
    AC_CONFIG_SUBDIRS([gmpc-lirc])
fi

#
# magnatune 
# 
AC_ARG_ENABLE(magnatune,
         [  --enable-magnatune],
         [ case "${enableval}" in
         yes)    enable_magnatune=yes;;
          no)    enable_magnatune=no;;
           *)    AC_MSG_ERROR([bad value ${enableval} for --enable-magnatune]);;
          esac],
         [enable_magnatune=yes])
 

AM_CONDITIONAL([MAGNATUNE_OPT], [test "$enable_magnatune" = yes])
if test x${enable_magnatune} = xyes; then
    AC_CONFIG_SUBDIRS([gmpc-magnatune])
fi

#
# shout 
# 
AC_ARG_ENABLE(shout,
         [  --enable-shout],
         [ case "${enableval}" in
         yes)    enable_shout=yes;;
          no)    enable_shout=no;;
           *)    AC_MSG_ERROR([bad value ${enableval} for --enable-shout]);;
          esac],
         [enable_shout=yes])
 

AM_CONDITIONAL([SHOUT_OPT], [test "$enable_shout" = yes])
if test x${enable_shout} = xyes; then
    AC_CONFIG_SUBDIRS([gmpc-shout])
fi
#
# mserver 
# 
AC_ARG_ENABLE(mserver,
         [  --enable-mserver],
         [ case "${enableval}" in
         yes)    enable_mserver=yes;;
          no)    enable_mserver=no;;
           *)    AC_MSG_ERROR([bad value ${enableval} for --enable-mserver]);;
          esac],
         [enable_mserver=yes])
 

AM_CONDITIONAL([MSERVER_OPT], [test "$enable_mserver" = yes])
if test x${enable_mserver} = xyes; then
    AC_CONFIG_SUBDIRS([gmpc-mserver])
fi
#
# last.fm-radio 
# 
#AC_ARG_ENABLE(lastfmradio,
#         [  --enable-lastfmradio],
#         [ case "${enableval}" in
#         yes)    enable_lastfmradio=yes;;
#          no)    enable_lastfmradio=no;;
#           *)    AC_MSG_ERROR([bad value ${enableval} for --enable-lastfmradio]);;
#          esac],
#         [enable_lastfmradio=yes])
 

#AM_CONDITIONAL([LASTFMRADIO_OPT], [test "$enable_lastfmradio" = yes])
#if test x${enable_lastfmradio} = xyes; then
#    AC_CONFIG_SUBDIRS([gmpc-lastfmradio])
#fi
#
# tagedit 
# 
AC_ARG_ENABLE(tagedit,
         [  --enable-tagedit],
         [ case "${enableval}" in
         yes)    enable_tagedit=yes;;
          no)    enable_tagedit=no;;
           *)    AC_MSG_ERROR([bad value ${enableval} for --enable-tagedit]);;
          esac],
         [enable_tagedit=yes])
 

AM_CONDITIONAL([TAGEDIT_OPT], [test "$enable_tagedit" = yes])
if test x${enable_tagedit} = xyes; then
    AC_CONFIG_SUBDIRS([gmpc-tagedit])
fi

#
# wikipedia 
# 
AC_ARG_ENABLE(wikipedia,
         [  --enable-wikipedia],
         [ case "${enableval}" in
         yes)    enable_wikipedia=yes;;
          no)    enable_wikipedia=no;;
           *)    AC_MSG_ERROR([bad value ${enableval} for --enable-wikipedia]);;
          esac],
         [enable_wikipedia=yes])
 

AM_CONDITIONAL([WIKIPEDIA_OPT], [test "$enable_wikipedia" = yes])
if test x${enable_wikipedia} = xyes; then
    AC_CONFIG_SUBDIRS([gmpc-wikipedia])
fi

#
# playlistsort 
# 
AC_ARG_ENABLE(playlistsort,
         [  --enable-playlistsort],
         [ case "${enableval}" in
         yes)    enable_playlistsort=yes;;
          no)    enable_playlistsort=no;;
           *)    AC_MSG_ERROR([bad value ${enableval} for --enable-playlistsort]);;
          esac],
         [enable_playlistsort=yes])
 

AM_CONDITIONAL([PLAYLISTSORT_OPT], [test "$enable_playlistsort" = yes])
if test x${enable_playlistsort} = xyes; then
    AC_CONFIG_SUBDIRS([gmpc-playlistsort])
fi

##
# Dynamic Playlist
##
#AC_ARG_ENABLE(dynamicplaylist,
#         [  --enable-dynamicplaylist],
#         [ case "${enableval}" in
#         yes)    enable_dynamicplaylist=yes;;
#          no)    enable_dynamicplaylist=no;;
#           *)    AC_MSG_ERROR([bad value ${enableval} for --enable-dynamicplaylist]);;
#          esac],
#         [enable_dynamicplaylist=yes])
         
         
#AM_CONDITIONAL([DYNAMICPLAYLIST_OPT], [test "$enable_dynamicplaylist" = yes])
#if test x${enable_dynamicplaylist} = xyes; then
#    AC_CONFIG_SUBDIRS([gmpc-dynamicplaylist])
#fi

##
# Lyricsplugin
##
AC_ARG_ENABLE(lyricsplugin,
         [  --enable-lyricsplugin],
         [ case "${enableval}" in
         yes)    enable_lyricsplugin=yes;;
          no)    enable_lyricsplugin=no;;
           *)    AC_MSG_ERROR([bad value ${enableval} for --enable-lyricsplugin]);;
          esac],
         [enable_lyricsplugin=yes])
         
         
AM_CONDITIONAL([LYRICSPLUGIN_OPT], [test "$enable_lyricsplugin" = yes])
if test x${enable_lyricsplugin} = xyes; then
    AC_CONFIG_SUBDIRS([gmpc-lyricsplugin])
fi

##
# albumview
##
AC_ARG_ENABLE(albumview,
         [  --enable-albumview],
         [ case "${enableval}" in
         yes)    enable_albumview=yes;;
          no)    enable_albumview=no;;
           *)    AC_MSG_ERROR([bad value ${enableval} for --enable-albumview]);;
          esac],
         [enable_albumview=yes])
         
         
AM_CONDITIONAL([ALBUMVIEW_OPT], [test "$enable_albumview" = yes])
if test x${enable_albumview} = xyes; then
    AC_CONFIG_SUBDIRS([gmpc-albumview])
fi

##
# Discogs 
##
AC_ARG_ENABLE(discogs,
         [  --enable-discogs],
         [ case "${enableval}" in
         yes)    enable_discogs=yes;;
          no)    enable_discogs=no;;
           *)    AC_MSG_ERROR([bad value ${enableval} for --enable-discogs]);;
          esac],
         [enable_discogs=yes])
         
         
AM_CONDITIONAL([DISCOGS_OPT], [test "$enable_discogs" = yes])
if test x${enable_discogs} = xyes; then
    AC_CONFIG_SUBDIRS([gmpc-discogs])
fi
##
# Fullscreen info 
##
AC_ARG_ENABLE(fullscreeninfo,
         [  --enable-fullscreeninfo],
         [ case "${enableval}" in
         yes)    enable_fullscreeninfo=yes;;
          no)    enable_fullscreeninfo=no;;
           *)    AC_MSG_ERROR([bad value ${enableval} for --enable-fullscreeninfo]);;
          esac],
         [enable_fullscreeninfo=yes])
         
         
AM_CONDITIONAL([FULLSCREENINFO_OPT], [test "$enable_fullscreeninfo" = yes])
if test x${enable_fullscreeninfo} = xyes; then
    AC_CONFIG_SUBDIRS([gmpc-fullscreeninfo])
fi


##
# MMKeys info 
##
AC_ARG_ENABLE(mmkeys,
         [  --enable-mmkeys],
         [ case "${enableval}" in
         yes)    enable_mmkeys=yes;;
          no)    enable_mmkeys=no;;
           *)    AC_MSG_ERROR([bad value ${enableval} for --enable-mmkeys]);;
          esac],
         [enable_mmkeys=yes])
         
         
AM_CONDITIONAL([MMKEYS_OPT], [test "$enable_mmkeys" = yes])
if test x${enable_mmkeys} = xyes; then
    AC_CONFIG_SUBDIRS([gmpc-mmkeys])
fi


AC_CONFIG_FILES([Makefile])

AC_OUTPUT


##
# Echo to the console what is updated
##

echo "======================================================"
if test x${enable_awn} = xyes; then
    echo "awn                               enabled"
else
    echo "awn                               disabled"
fi
if test x${enable_lastfm} = xyes; then
    echo "last.fm                           enabled"
else
    echo "last.fm                           disabled"
fi
if test x${enable_coveramazon} = xyes; then
    echo "Cover amazon                      enabled"
else
if test x${enable_lyricwiki} = xno; then
    echo "Cover amazon                        disabled"
else
    echo "Cover amazon                        unavailable"
fi
fi

if test x${enable_lyrics} = xyes; then
    echo "Lyrics                            enabled"
else
    echo "Lyrics                            disabled"
fi
if test x${enable_mdcover} = xyes; then
    echo "mdcover                           enabled"
else
    echo "mdcover                           disabled"
fi
if test x${enable_lyricwiki} = xyes; then
    echo "lyric wiki                        enabled"
else
if test x${enable_lyricwiki} = xno; then
    echo "lyric wiki                        disabled"
else
    echo "lyric wiki                        unavailable"
fi
fi
if test x${enable_extraplaylist} = xyes; then
    echo "extraplaylist                     enabled"
else
    echo "extraplaylist                     disabled"
fi
if test x${enable_avahi} = xyes; then
    echo "avahi                             enabled"
else
    echo "avahi                             disabled"
fi
if test x${enable_libnotify} = xyes; then
    echo "libnotify                         enabled"
else
    echo "libnotify                         disabled"
fi
if test x${enable_alarm} = xyes; then
    echo "alarm                             enabled"
else
    echo "alarm                             disabled"
fi
if test x${enable_jamendo} = xyes; then
    echo "jamendo                           enabled"
else
    echo "jamendo                           disabled"
fi
if test x${enable_lirc} = xyes; then
    echo "lirc                              enabled"
else
    echo "lirc                              disabled"
fi
if test x${enable_magnatune} = xyes; then
    echo "magnatune                         enabled"
else
    echo "magnatune                         disabled"
fi
if test x${enable_shout} = xyes; then
    echo "shout                             enabled"
else
    echo "shout                             disabled"
fi
if test x${enable_mserver} = xyes; then
    echo "mserver                           enabled"
else
    echo "mserver                           disabled"
fi
#if test x${enable_lastfmradio} = xyes; then
#    echo "lastfmradio                       enabled"
#else
    echo "lastfmradio                       unavailable"
#fi
if test x${enable_tagedit} = xyes; then
    echo "tagedit                           enabled"
else
    echo "tagedit                           disabled"
fi

if test x${enable_wikipedia} = xyes; then
    echo "wikipedia                         enabled"
else
    echo "wikipedia                         disabled"
fi
if test x${enable_playlistsort} = xyes; then
    echo "playlistsort                      enabled"
else
    echo "playlistsort                      disabled"
fi
#if test x${enable_dynamicplaylist} = xyes; then
    echo "Dynamic Playlist                  unavailable"
#else
#    echo "Dynamic Playlist                  disabled"
#fi

if test x${enable_lyricsplugin} = xyes; then
    echo "Lyricsplugin                      enabled"
else
    echo "Lyricsplugin                      disabled"
fi


if test x${enable_albumview} = xyes; then
    echo "Albumview                         enabled"
else
    echo "Albumview                         disabled"
fi

if test x${enable_discogs} = xyes; then
    echo "Discogs Playlist                  enabled"
else
    echo "Discogs Playlist                  disabled"
fi

if test x${enable_fullscreeninfo} = xyes; then
    echo "Fullscreen info                   enabled"
else
    echo "Fullscreen info                   disabled"
fi

if test x${enable_mmkeys} = xyes; then
    echo "MMKeys info                   enabled"
else
    echo "MMKeys info                   disabled"
fi
