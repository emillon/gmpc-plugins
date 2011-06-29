/* Gnome Music Player Client Multimedia Keys plugin (gmpc-mmkeys)
 * Copyright (C) 2009 Qball Cow <qball@sarine.nl>
 * Project homepage: http://gmpc.wikia.com/
 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
using GLib;
using Gmpc;
using Gmpc.Plugin;

[DBus (name = "org.gnome.SettingsDaemon.MediaKeys")]
interface MediaKeys : GLib.Object {
    public abstract void GrabMediaPlayerKeys (string application, uint32 time) throws DBus.Error;
    public abstract void ReleaseMediaPlayerKeys (string application) throws DBus.Error;
    public signal void MediaPlayerKeyPressed (string application, string keys);
}

public class MMKeys : Gmpc.Plugin.Base {
    public const int[] pversion = {0,0,0};
    private DBus.Connection conn;
    private MediaKeys keys;

    public override void set_enabled(bool enabled) {
        var old = this.get_enabled();
        if(this.get_name() != null)
            Gmpc.config.set_int(this.get_name(), "enabled", (int)enabled); 
        if(enabled == true && old == false) {
            try{
                keys.GrabMediaPlayerKeys("gmpc", (uint)0);
            } catch (DBus.Error error) {
               warning("Failed to grab media keys: %s\n", error.message); 
            }
        }else if (enabled == false && old == true){
            try{
                keys.ReleaseMediaPlayerKeys("gmpc");
            } catch (DBus.Error error) {
               warning("Failed to release media keys: %s\n", error.message); 
            }
        }
    }
    /* Name */
    public override weak string get_name() {
        return "Gnome multimedia keys plugin";
    }
    /* Version */
    public override  weak int[] get_version() {
        return pversion;
    }
    private void callback (MediaKeys mkeys, string application, string keys) {
        if(this.get_enabled() == false) return;
        if(application != "gmpc") return;
        if(keys == "Play") {
            if(MPD.Player.get_state(server) == MPD.Player.State.PLAY)
                MPD.Player.pause(server);
            else
                MPD.Player.play(server);
        }
        else if(keys == "Pause") MPD.Player.pause(server);
        else if(keys == "Next") MPD.Player.next(server);
        else if(keys == "Previous") MPD.Player.prev(server);
        else if(keys == "Stop")  MPD.Player.stop(server);
    }
    construct {
        /* Set the plugin as an internal one and of type pl_browser */
        this.plugin_type = 4; 
        conn =  DBus.Bus.get (DBus.BusType.SESSION);
        keys = (MediaKeys) conn.get_object ( "org.gnome.SettingsDaemon",
                "/org/gnome/SettingsDaemon/MediaKeys");

        if(this.get_enabled()) {
            try {
                keys.GrabMediaPlayerKeys("gmpc", (uint)0);
            } catch (DBus.Error error) {
                warning("Failed to grab media keys: %s\n", error.message); 
            }
        }
        keys.MediaPlayerKeyPressed += callback;
    }
    ~MMKeys() {
        if(this.get_enabled()) {
            try {
                keys.ReleaseMediaPlayerKeys("gmpc");
            } catch (DBus.Error error) {
               warning("Failed to release media keys: %s\n", error.message); 
            }
        }
    }
}
[ModuleInit]
public Type plugin_get_type () {
        return typeof (MMKeys);
}
