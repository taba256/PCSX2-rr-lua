/*
 *  Copyright (C) 2002-2004  GSsoft Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __LINUX_H__
#define __LINUX_H__
#endif

void OnToggle_advopts(GtkCellRendererToggle *cell, gchar *path, gpointer user_data);

#define is_checked(main_widget, widget_name) (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(main_widget, widget_name)))) 
#define set_checked(main_widget,widget_name, state) gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(main_widget, widget_name)), state)
