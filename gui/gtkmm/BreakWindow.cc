// BreakWindow.cc --- base class for the break windows
//
// Copyright (C) 2001, 2002 Rob Caelers <robc@krandor.org>
// All rights reserved.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2, or (at your option)
// any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//

static const char rcsid[] = "$Id$";

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "debug.hh"

#include "preinclude.h"

#include <gtkmm.h>

#include "BreakWindow.hh"
#include "WindowHints.hh"
#include "Frame.hh"


const int MARGIN = 20;


//! Constructor
/*!
 *  \param control The controller.
 */
BreakWindow::BreakWindow() :
  Gtk::Window (Gtk::WINDOW_POPUP),
#ifdef HAVE_X
  grab_wanted(false),
#endif
  avoid_wanted(false),
  frame(NULL),
  border_width(0),
  grab_handle(NULL)
{
  Gtk::Window::set_border_width(0);
}


//! Adds child
void
BreakWindow::add(Gtk::Widget& widget)
{
  if (! frame)
    {
      frame = manage(new Frame());
      frame->set_border_width(Gtk::Window::get_border_width());
      frame->set_frame_style(Frame::STYLE_BREAK_WINDOW);
      set_border_width(border_width);
      Gtk::Window::add(*frame);
    }
  frame->add(widget);
}

//! Border 
void
BreakWindow::set_border_width(guint width)
{
  border_width = width;
  if (frame)
    frame->set_border_width(border_width);
}

//! Destructor.
BreakWindow::~BreakWindow()
{
  TRACE_ENTER("BreakWindow::~BreakWindow");
  ungrab();
  TRACE_EXIT();
}



//! Centers window.
void
BreakWindow::center()
{
  TRACE_ENTER("BreakWindow::center");
  set_position(Gtk::WIN_POS_CENTER_ALWAYS);
}


//! Grabs the pointer and the keyboard.
bool
BreakWindow::grab()
{
  Glib::RefPtr<Gdk::Window> window = get_window();
  GdkWindow *gdkWindow = window->gobj();
#ifdef HAVE_X
  grab_wanted = true;
#endif
  if (! grab_handle)
    {
      grab_handle = WindowHints::grab(gdkWindow);
#ifdef HAVE_X
      if (! grab_handle)
	{
	  Glib::signal_timeout().connect(SigC::slot(*this, &BreakWindow::on_grab_retry_timer), 2000);
	}
#endif
    }
  return grab_handle != NULL;
}


//! Releases the pointer and keyboard grab
void
BreakWindow::ungrab()
{
#ifdef HAVE_X
  grab_wanted = false;
#endif
  if (grab_handle)
    {
      WindowHints::ungrab(grab_handle);
      grab_handle = NULL;
    }
}


#ifdef HAVE_X
//! Reattempt to get the grab
bool
BreakWindow::on_grab_retry_timer()
{
  if (grab_wanted)
    {
      return !grab();
    }
  else
    {
      return false;
    }
}
#endif


void
BreakWindow::set_avoid_pointer(bool avoid_pointer)
{
  Gdk::EventMask events;

  avoid_wanted = avoid_pointer;
#if defined(HAVE_X)
  events = Gdk::ENTER_NOTIFY_MASK;
#elif defined(WIN32)
  events = (Gdk::POINTER_MOTION_MASK
            |Gdk::POINTER_MOTION_HINT_MASK);
#endif
  
  if (avoid_pointer)
    {
      add_events(events);
    }
  else
    {
      set_events(get_events() & ~events);
    }
}

#ifdef WIN32
bool
BreakWindow::on_motion_notify_event(GdkEventMotion *event)
{
  avoid_pointer(event->x, event->y);
  return Gtk::Window::on_motion_notify_event(event);
}

#else

//! GDK EventNotifyEvent notification.
bool
BreakWindow::on_enter_notify_event(GdkEventCrossing *event)
{
  avoid_pointer(event->x, event->y);
  return Gtk::Window::on_enter_notify_event(event);
}
#endif

void
BreakWindow::avoid_pointer(int px, int py)
{
  TRACE_ENTER_MSG("BreakWindow::avoid_pointer", "x="<<px<<",y="<<py);

  if (! avoid_wanted)
    return;
  
  Glib::RefPtr<Gdk::Window> window = get_window();
    
  int winx, winy, width, height, wind;
  window->get_geometry(winx, winy, width, height, wind);
  get_position(winx, winy);
  
  gdouble x = px;
  gdouble y = py;

  int deltax = 0;
  int deltay = 0;
  const int jump_x = 2*width;
  const int jump_y = 2*height;
    
  if (y < (0.2 * height))
    deltay = jump_y;
  else if (y > (0.8 * height))
    deltay = -jump_y;
        
  if (x < (0.2 * width))
    deltax = jump_x;
  else if (x > 0.8 * width)
    deltax = -jump_x;
  else if (! deltay)
    {
      if (x > 0.5*width)
        deltax = -jump_x;
      else
        deltax = jump_x;
    }

  int screen_width = gdk_screen_width();
  int screen_height = gdk_screen_height();
  
  winx += deltax;
  winy += deltay;

  int right_margin = screen_width - width - MARGIN;
  int bottom_margin = screen_height - height - MARGIN;
  
  if (winx < MARGIN)
    winx = right_margin;
  else if (winx > right_margin)
    winx = MARGIN;
             
  if (winy < MARGIN)
    winy = bottom_margin;
  else if (winy > bottom_margin)
    winy = MARGIN;

  TRACE_MSG(winx << " " << winy);
  set_position(Gtk::WIN_POS_NONE);
  move(winx, winy);

  TRACE_EXIT();
}