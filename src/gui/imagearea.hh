/* This code is directly derived from the gtkdisp2.cc program included in the 
 * VIPS distribution; credits go therefore to the VIPS authors.
 *
 * 8-bit RGB images only, though it would be easy to fix this.
 *
 */

/*

    Copyright (C) 2014 Ferrero Andrea

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.


 */

/*

    These files are distributed with PhotoFlow - http://aferrero2707.github.io/PhotoFlow/

 */

#ifndef IMAGE_AREA_HH
#define IMAGE_AREA_HH

#include <stdio.h>
#include <iostream>

#include <queue>

#include <gtkmm.h>

//#include <vips/vips>

#include "../base/photoflow.hh"
#include "../base/pipeline.hh"
#include "../base/image.hh"

#include "../operations/image_reader.hh"
#include "../operations/convert2srgb.hh"
#include "../operations/uniform.hh"
#include "../operations/invert.hh"
#include "../operations/blender.hh"
#include "../operations/convertformat.hh"

#include "../gui/operations/imageread_config.hh"

#include "doublebuffer.hh"


/*
  The ImageArea performs the image update aynchronously, inside a dedicated thread.
  Upon a drawing request from the Gtk system (on_expose_event or on_draw)
  the rectangles are inserted in a waiting queue and the processing thread is waken.
  The processing thread takes each rectangle, updates the corresponding image region
  and installs an idle callback function that takes care of drawing the region on the display.
  The processing thread then waits until the region is drawn before processing the
  next rectangle.
 */

namespace PF
{

class ImageArea : public PipelineSink, public Gtk::DrawingArea
{

#ifdef GTKMM_2
  Gtk::Adjustment* hadj;
  Gtk::Adjustment* vadj;
#endif
#ifdef GTKMM_3
  Glib::RefPtr<Gtk::Adjustment> hadj;
  Glib::RefPtr<Gtk::Adjustment> vadj;
#endif

  /* The derived image we paint to the screen.
   */
  VipsImage* display_image;
  VipsImage* outimg;

  unsigned int xoffset, yoffset;

  /* The region we prepare from to draw the pixels,
   */
  VipsRegion* region;

  DoubleBuffer double_buffer;
  PixelBuffer temp_buffer;

  GCond* draw_done;
  GMutex* draw_mutex;

  bool draw_requested;

  //Glib::Dispatcher signal_queue_draw;

  /* The cache mask. 
   */
  VipsImage* mask;

  /* Read from the cache mask with this. 
   */
  VipsRegion* mask_region;

  PF::ProcessorBase* convert2srgb;

  PF::Processor<PF::UniformPar,PF::Uniform>* uniform;
  PF::Processor<PF::BlenderPar,PF::BlenderProc>* maskblend;

  PF::ProcessorBase* invert;

  PF::ProcessorBase* convert_format;

  bool display_merged;
  int active_layer;

	float shrink_factor;

  long int pending_pixels;

  /* We send this packet of data from the bg worker thread to the main GUI
   * thread when a tile has been calculated.
   */
  typedef struct {
    ImageArea * image_area;
    VipsRect rect;
    guchar* buf;
    int lsk;
  } Update;

  static gboolean set_size_cb (Update * update);

  static gboolean queue_draw_cb (Update * update);

  /* Come here from the vips_sink_screen() background thread when a tile has 
   * been calculated. 
   *
   * We can't paint the screen directly since the main GUI thread might be 
   * doing something. Instead, we add an idle callback which will be
   * run by the main GUI thread when it next hits the mainloop.

  static void
  sink_notify (VipsImage *image, VipsRect *rect, void *client)
  {
    ImageArea * image_area = (ImageArea *) client;
    Update * update = g_new (Update, 1);

    update->rect = *rect;
    update->image_area = image_area;

    g_idle_add ((GSourceFunc) render_cb, update);
  }
  */

public:

  ImageArea( Pipeline* v );
  virtual ~ImageArea();

  unsigned int get_xoffset() { return xoffset; }
  unsigned int get_yoffset() { return yoffset; }

	VipsImage* get_display_image() { return display_image; }

  DoubleBuffer& get_double_buffer() { return double_buffer; }

#ifdef GTKMM_2
  //void expose_rect (const VipsRect& area);
  bool on_expose_event (GdkEventExpose * event);
#endif

#ifdef GTKMM_3
  //void expose_rect (const VipsRect& area, const Cairo::RefPtr<Cairo::Context>& cr);
  bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr);
#endif
  void submit_area( const VipsRect& area );
  void process_start( const VipsRect& area );
  void process_area( const VipsRect& area );
  void process_end( const VipsRect& area );
  void draw_area();

  Glib::RefPtr< Gdk::Pixbuf > modify_preview();

	float get_shrink_factor() { return shrink_factor; }
	void set_shrink_factor( float val ) { shrink_factor = val; }

  void set_adjustments( 
#ifdef GTKMM_2
		       Gtk::Adjustment* h, Gtk::Adjustment* v
#endif
#ifdef GTKMM_3
		       Glib::RefPtr<Gtk::Adjustment> h,
		       Glib::RefPtr<Gtk::Adjustment> v
#endif
			)
  {
    hadj = h; vadj = v;
  }

#ifdef GTKMM_2
  Gtk::Adjustment*
#endif
#ifdef GTKMM_3
  Glib::RefPtr<Gtk::Adjustment>
#endif
  get_hadj() { return hadj; }
#ifdef GTKMM_2
  Gtk::Adjustment*
#endif
#ifdef GTKMM_3
  Glib::RefPtr<Gtk::Adjustment>
#endif
  get_vadj() { return vadj; }

  void update( VipsRect* area );

  void sink( const VipsRect& area );

  void set_active_layer( int id ) { 
    int old_id = active_layer;
    active_layer = id; 
    if( !display_merged && (old_id != active_layer) ) {
      //update( NULL );
			if( get_pipeline() && get_pipeline()->get_image() )
				get_pipeline()->get_image()->update();
		}
  }
  int get_active_layer() { return active_layer; }

  void set_display_merged( bool val )
  {
    bool old_val = display_merged;
    display_merged = val;
    if( display_merged != old_val ) {
      //update( NULL );
			if( get_pipeline() && get_pipeline()->get_image() )
				get_pipeline()->get_image()->update();
		}
  }
  bool get_display_merged() { return display_merged; }

  virtual void on_realize() 
  {
    Gtk::DrawingArea::on_realize();

    //get_window()->set_back_pixmap( Glib::RefPtr<Gdk::Pixmap>(), FALSE );
    //set_double_buffered( TRUE );

    Gtk::Allocation allocation = get_allocation();
#ifndef NDEBUG
    std::cout<<"DrawingArea size: "<<allocation.get_width()<<","
	     <<allocation.get_height()<<std::endl;
#endif
  }
};

}

#endif
