/* 
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

#include "../base/exif_data.hh"
#include "../../operations/raw_preprocessor.hh"
#include "../../operations/raw_output.hh"
#include "../../operations/raw_developer.hh"

/* We need C linkage for this.
 */
#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

#include "../dt/common/colorspaces.h"

#ifdef __cplusplus
}
#endif /*__cplusplus*/

#include "../dt/external/wb_presets.c"

#include "raw_developer_config.hh"


#ifndef MIN
#define MIN( a, b ) ((a<b) ? a : b)
#endif
#define MIN3( a, b, c ) MIN(a,MIN(b,c))

#ifndef MAX
#define MAX( a, b ) ((a>b) ? a : b)
#endif
#define MAX3( a, b, c ) MAX(a,MAX(b,c))


bool PF::WBSelector::check_value( int id, const std::string& name, const std::string& val )
{
  if( id < 3 ) return true;
  std::cout<<"WBSelector::check_value(): maker="<<maker<<" model="<<model<<std::endl;
  for(int i = 0; i < wb_preset_count; i++) {
    //std::cout<<"  wb_preset[i].make="<<wb_preset[i].make<<" wb_preset[i].model="<<wb_preset[i].model<<std::endl;
    if( maker == wb_preset[i].make && model == wb_preset[i].model ) {
      //std::cout<<"    val="<<val<<" wb_preset[i].name="<<wb_preset[i].name<<std::endl;
      if( val == wb_preset[i].name ) {
        return true;
      }
    }
  }
  return false;
}



PF::RawDeveloperConfigDialog::RawDeveloperConfigDialog( PF::Layer* layer ):
  OperationConfigDialog( layer, "Raw Developer" ),
  wbModeSelector( this, "wb_mode", "WB mode: ", 0 ),
  wbRedSlider( this, "wb_red", "Red WB mult.", 1, 0, 10, 0.05, 0.1, 1),
  wbGreenSlider( this, "wb_green", "Green WB mult.", 1, 0, 10, 0.05, 0.1, 1),
  wbBlueSlider( this, "wb_blue", "Blue WB mult.", 1, 0, 10, 0.05, 0.1, 1),
  wbRedCorrSlider( this, "camwb_corr_red", "Red WB correction", 1, 0, 10, 0.05, 0.1, 1),
  wbGreenCorrSlider( this, "camwb_corr_green", "Green WB correction", 1, 0, 10, 0.05, 0.1, 1),
  wbBlueCorrSlider( this, "camwb_corr_blue", "Blue WB correction", 1, 0, 10, 0.05, 0.1, 1),
  wb_target_L_slider( this, "wb_target_L", "Target: ", 50, 0, 1000000, 0.05, 0.1, 1),
  wb_target_a_slider( this, "wb_target_a", "a: ", 10, -1000000, 1000000, 0.05, 0.1, 1),
  wb_target_b_slider( this, "wb_target_b", "b: ", 12, -1000000, 1000000, 0.05, 0.1, 1),
  demoMethodSelector( this, "demo_method", "Demosaicing method: ", PF::PF_DEMO_AMAZE ),
  fcsSlider( this, "fcs_steps", "False color suppression steps", 1, 0, 4, 1, 1, 1 ),
  exposureSlider( this, "exposure", "Exp. compensation", 0, -5, 5, 0.05, 0.5 ),
  profileModeSelector( this, "profile_mode", "Color conversion mode: ", 0 ),
  camProfOpenButton(Gtk::Stock::OPEN),
  gammaModeSelector( this, "gamma_mode", "Raw gamma: ", 0 ),
  inGammaLinSlider( this, "gamma_lin", "Gamma linear", 0, 0, 100000, 0.05, 0.1, 1),
  inGammaExpSlider( this, "gamma_exp", "Gamma exponent", 2.2, 0, 100000, 0.05, 0.1, 1),
  outProfileModeSelector( this, "out_profile_mode", "Output profile: ", 1 ),
  outProfOpenButton(Gtk::Stock::OPEN)
{
  wbControlsBox.pack_start( wbModeSelector, Gtk::PACK_SHRINK );

  wb_target_L_slider.set_passive( true );
  wb_target_a_slider.set_passive( true );
  wb_target_b_slider.set_passive( true );
  //wbTargetBox.pack_start( wb_target_L_slider, Gtk::PACK_SHRINK );
  wbTargetBox.pack_start( wb_target_a_slider, Gtk::PACK_SHRINK );
  wbTargetBox.pack_start( wb_target_b_slider, Gtk::PACK_SHRINK );
  wbControlsBox.pack_start( wbTargetBox );
  wbControlsBox.pack_start( wb_best_match_label, Gtk::PACK_SHRINK );

  wbControlsBox.pack_start( wbRedSlider );
  wbControlsBox.pack_start( wbGreenSlider );
  wbControlsBox.pack_start( wbBlueSlider );
  wbControlsBox.pack_start( wbRedCorrSlider );
  wbControlsBox.pack_start( wbGreenCorrSlider );
  wbControlsBox.pack_start( wbBlueCorrSlider );

  exposureControlsBox.pack_start( exposureSlider );

  demoControlsBox.pack_start( demoMethodSelector );
  demoControlsBox.pack_start( fcsSlider );

  profileModeSelectorBox.pack_start( profileModeSelector, Gtk::PACK_SHRINK );
  outputControlsBox.pack_start( profileModeSelectorBox, Gtk::PACK_SHRINK );

  camProfLabel.set_text( "camera profile name:" );
  camProfVBox.pack_start( camProfLabel );
  camProfVBox.pack_start( camProfFileEntry );
  camProfHBox.pack_start( camProfVBox );
  camProfHBox.pack_start( camProfOpenButton, Gtk::PACK_SHRINK );
  outputControlsBox.pack_start( camProfHBox );

  gammaModeVBox.pack_start( gammaModeSelector );
  //gammaModeVBox.pack_start( inGammaLinSlider );
  gammaModeVBox.pack_start( inGammaExpSlider );
  gammaModeHBox.pack_start( gammaModeVBox, Gtk::PACK_SHRINK );
  outputControlsBox.pack_start( gammaModeHBox );

  outProfileModeSelectorBox.pack_start( outProfileModeSelector, Gtk::PACK_SHRINK );
  outputControlsBox.pack_start( outProfileModeSelectorBox, Gtk::PACK_SHRINK );

  outProfLabel.set_text( "output profile name:" );
  outProfVBox.pack_start( outProfLabel );
  outProfVBox.pack_start( outProfFileEntry );
  outProfHBox.pack_start( outProfVBox );
  outProfHBox.pack_start( outProfOpenButton, Gtk::PACK_SHRINK );
  outputControlsBox.pack_start( outProfHBox );


  notebook.append_page( wbControlsBox, "White balance" );
  notebook.append_page( exposureControlsBox, "Exposure" );
  notebook.append_page( demoControlsBox, "Demosaicing" );
  notebook.append_page( outputControlsBox, "Output" );
    
  add_widget( notebook );


  camProfFileEntry.signal_activate().
    connect(sigc::mem_fun(*this,
			  &RawDeveloperConfigDialog::on_cam_filename_changed));
  camProfOpenButton.signal_clicked().connect(sigc::mem_fun(*this,
							   &RawDeveloperConfigDialog::on_cam_button_open_clicked) );

  outProfFileEntry.signal_activate().
    connect(sigc::mem_fun(*this,
			  &RawDeveloperConfigDialog::on_out_filename_changed));
  outProfOpenButton.signal_clicked().connect(sigc::mem_fun(*this,
							   &RawDeveloperConfigDialog::on_out_button_open_clicked) );

  show_all_children();
}




void PF::RawDeveloperConfigDialog::do_update()
{
  std::cout<<"RawDeveloperConfigDialog::do_update() called."<<std::endl;
  if( get_layer() && get_layer()->get_image() && 
      get_layer()->get_processor() &&
      get_layer()->get_processor()->get_par() ) {
    PF::RawDeveloperPar* par = 
      dynamic_cast<PF::RawDeveloperPar*>(get_layer()->get_processor()->get_par());
    if( !par ) return;

    PropertyBase* prop = par->get_property( "wb_mode" );
    if( !prop )  return;

    PF::Image* image = get_layer()->get_image();
    PF::Pipeline* pipeline = image->get_pipeline(0);
    PF::PipelineNode* node = NULL;
    PF::PipelineNode* inode = NULL;
    std::string maker, model;
    if( pipeline ) node = pipeline->get_node( get_layer()->get_id() );
    if( node ) inode = pipeline->get_node( node->input_id );
    if( inode && inode->image) {
      size_t blobsz;
      PF::exif_data_t* exif_data;
      if( !vips_image_get_blob( inode->image, PF_META_EXIF_NAME,(void**)&exif_data,&blobsz ) &&
          blobsz == sizeof(PF::exif_data_t) ) {
        char makermodel[1024];
        char *tmodel = makermodel;
        dt_colorspaces_get_makermodel_split(makermodel, sizeof(makermodel), &tmodel,
            exif_data->exif_maker, exif_data->exif_model );
        maker = makermodel;
        model = tmodel;
        wbModeSelector.set_maker_model( maker, model );
        std::cout<<"RawDeveloperConfigDialog::do_update(): maker="<<maker<<" model="<<model<<std::endl;
      }
    }

    //std::cout<<"PF::RawDeveloperConfigDialog::do_update() called."<<std::endl;

		if( wbTargetBox.get_parent() == &wbControlsBox )
			wbControlsBox.remove( wbTargetBox );
		if( wb_best_match_label.get_parent() == &wbControlsBox )
			wbControlsBox.remove( wb_best_match_label );

		if( wbRedSlider.get_parent() == &wbControlsBox )
			wbControlsBox.remove( wbRedSlider );
		if( wbGreenSlider.get_parent() == &wbControlsBox )
			wbControlsBox.remove( wbGreenSlider );
		if( wbBlueSlider.get_parent() == &wbControlsBox )
			wbControlsBox.remove( wbBlueSlider );

		if( wbRedCorrSlider.get_parent() == &wbControlsBox )
			wbControlsBox.remove( wbRedCorrSlider );
		if( wbGreenCorrSlider.get_parent() == &wbControlsBox )
			wbControlsBox.remove( wbGreenCorrSlider );
		if( wbBlueCorrSlider.get_parent() == &wbControlsBox )
			wbControlsBox.remove( wbBlueCorrSlider );

    switch( prop->get_enum_value().first ) {
    case PF::WB_SPOT:
			if( wbTargetBox.get_parent() == &wbControlsBox )
				wbControlsBox.remove( wbTargetBox );
			if( wb_best_match_label.get_parent() == &wbControlsBox )
				wbControlsBox.remove( wb_best_match_label );
			if( wbRedSlider.get_parent() != &wbControlsBox )
				wbControlsBox.pack_start( wbRedSlider, Gtk::PACK_SHRINK );
			if( wbGreenSlider.get_parent() != &wbControlsBox )
				wbControlsBox.pack_start( wbGreenSlider, Gtk::PACK_SHRINK );
			if( wbBlueSlider.get_parent() != &wbControlsBox )
				wbControlsBox.pack_start( wbBlueSlider, Gtk::PACK_SHRINK );
			break;
    case PF::WB_COLOR_SPOT:
			if( wbTargetBox.get_parent() != &wbControlsBox )
				wbControlsBox.pack_start( wbTargetBox, Gtk::PACK_SHRINK );
			if( wb_best_match_label.get_parent() != &wbControlsBox )
				wbControlsBox.pack_start( wb_best_match_label, Gtk::PACK_SHRINK );
			if( wbRedSlider.get_parent() != &wbControlsBox )
				wbControlsBox.pack_start( wbRedSlider, Gtk::PACK_SHRINK );
			if( wbGreenSlider.get_parent() != &wbControlsBox )
				wbControlsBox.pack_start( wbGreenSlider, Gtk::PACK_SHRINK );
			if( wbBlueSlider.get_parent() != &wbControlsBox )
				wbControlsBox.pack_start( wbBlueSlider, Gtk::PACK_SHRINK );
			break;
    default:
      if( wbRedCorrSlider.get_parent() != &wbControlsBox )
        wbControlsBox.pack_start( wbRedCorrSlider, Gtk::PACK_SHRINK );
      if( wbGreenCorrSlider.get_parent() != &wbControlsBox )
        wbControlsBox.pack_start( wbGreenCorrSlider, Gtk::PACK_SHRINK );
      if( wbBlueCorrSlider.get_parent() != &wbControlsBox )
        wbControlsBox.pack_start( wbBlueCorrSlider, Gtk::PACK_SHRINK );
      break;
		}

    prop = par->get_property( "cam_profile_name" );
    if( !prop )  return;
    std::string filename = prop->get_str();
    camProfFileEntry.set_text( filename.c_str() );

    prop = par->get_property( "out_profile_name" );
    if( !prop )  return;
    filename = prop->get_str();
    outProfFileEntry.set_text( filename.c_str() );

    prop = par->get_property( "profile_mode" );
    if( !prop )  return;

    if( camProfHBox.get_parent() == &outputControlsBox )
      outputControlsBox.remove( camProfHBox );

    if( outProfHBox.get_parent() == &outputControlsBox )
      outputControlsBox.remove( outProfHBox );

    if( gammaModeHBox.get_parent() == &outputControlsBox )
      outputControlsBox.remove( gammaModeHBox );

    switch( prop->get_enum_value().first ) {
    case PF::IN_PROF_NONE:
      outputControlsBox.pack_start( gammaModeHBox, Gtk::PACK_SHRINK );
      break;
    case PF::IN_PROF_MATRIX:
      outputControlsBox.pack_start( outProfHBox, Gtk::PACK_SHRINK );
      break;
    case PF::IN_PROF_ICC:
      outputControlsBox.pack_start( gammaModeHBox, Gtk::PACK_SHRINK );
      outputControlsBox.pack_start( camProfHBox, Gtk::PACK_SHRINK );
      outputControlsBox.pack_start( outProfHBox, Gtk::PACK_SHRINK );
      break;
    }
  }
  OperationConfigDialog::do_update();
}


/*
void PF::RawDeveloperConfigDialog::pointer_press_event( int button, double x, double y, int mod_key )
{
  if( button != 1 ) return;
}
*/

bool PF::RawDeveloperConfigDialog::pointer_release_event( int button, double sx, double sy, int mod_key )
{
  if( button != 1 ) return false;

  if( wbModeSelector.get_prop() &&
      wbModeSelector.get_prop()->is_enum() &&
      (wbModeSelector.get_prop()->get_enum_value().first == (int)PF::WB_SPOT) ) {
    double x = sx, y = sy, w = 1, h = 1;
    screen2layer( x, y, w, h );
    spot_wb( x, y );
  }

  if( wbModeSelector.get_prop() &&
      wbModeSelector.get_prop()->is_enum() &&
      (wbModeSelector.get_prop()->get_enum_value().first == (int)PF::WB_COLOR_SPOT) ) {
    double x = sx, y = sy, w = 1, h = 1;
    screen2layer( x, y, w, h );
    color_spot_wb( x, y );
  }

  return false;
}



void PF::RawDeveloperConfigDialog::spot_wb( double x, double y )
{
  // Get the layer associated to this operation
  PF::Layer* l = get_layer();
  if( !l ) return;

  // Get the image the layer belongs to
  PF::Image* img = l->get_image();
  if( !img ) return;
  
  // Get the default pipeline of the image 
  // (it is supposed to be at 1:1 zoom level 
  // and floating point accuracy)
  PF::Pipeline* pipeline = img->get_pipeline( 0 );
  if( !pipeline ) return;

	// Make sure the first pipeline is up-to-date
	img->update( pipeline, true );
  img->unlock();

  // Get the node associated to the layer
  PF::PipelineNode* node = pipeline->get_node( l->get_id() );
  if( !node ) return;

  // Finally, get the underlying VIPS image associated to the layer
  VipsImage* image = node->image;
  if( !image ) return;

  
  float rgb_check[3] = { 0, 0, 0 };
  float rgb_prev[3] = { 1000, 1000, 1000 };
  for( int i = 0; i < 100; i++ ) {
    // Now we have to process a small portion of the image 
    // to get the corresponding Lab values
    VipsImage* spot;
    int left = (int)x-3;
    int top = (int)y-3;
    int width = 7;
    int height = 7;

    float rgb_avg[3] = {0, 0, 0};
		std::vector<float> values;

    std::cout<<"RawDeveloperConfigDialog: getting spot WB ("<<x<<","<<y<<")"<<std::endl;
		img->sample( l->get_id(), x, y, 7, NULL, values );
		if( values.size() != 3 ) {
			std::cout<<"RawDeveloperConfigDialog::pointer_relese_event(): values.size() "
							 <<values.size()<<" (!= 3)"<<std::endl;
			return;
		}
		rgb_avg[0] = values[0];
		rgb_avg[1] = values[1];
		rgb_avg[2] = values[2];


    float rgb_out[3] = {0, 0, 0};
    float Lab_in[3] = {0, 0, 0};
    float Lab_out[3] = {0, 0, 0};
    float Lab_wb[3] = {
      static_cast<float>(wb_target_L_slider.get_adjustment()->get_value()),
      static_cast<float>(wb_target_a_slider.get_adjustment()->get_value()),
      static_cast<float>(wb_target_b_slider.get_adjustment()->get_value())
    };

    const float epsilon = 1.0e-5;
    float wb_red_mul;
    float wb_green_mul;
    float wb_blue_mul;

		// The target color is gray, so we simply neutralize the spot value
		// The green channel is kept fixed and the other two are scaled to 
		// the green value
		wb_red_mul = rgb_avg[1]/rgb_avg[0];
		wb_blue_mul = rgb_avg[1]/rgb_avg[2];
		wb_green_mul = 1;

    PropertyBase* wb_red_prop = wbRedSlider.get_prop();
    PropertyBase* wb_green_prop = wbGreenSlider.get_prop();
    PropertyBase* wb_blue_prop = wbBlueSlider.get_prop();
    if( wb_red_prop && wb_green_prop && wb_blue_prop ) {
      float wb_red_in;
      float wb_green_in;
      float wb_blue_in;
      wb_red_prop->get( wb_red_in );
      wb_green_prop->get( wb_green_in );
      wb_blue_prop->get( wb_blue_in );
      float wb_red_out = wb_red_mul*wb_red_in;
      float wb_green_out = wb_green_mul*wb_green_in;
      float wb_blue_out = wb_blue_mul*wb_blue_in;
      float scale = (wb_red_out+wb_green_out+wb_blue_out)/3.0f;
      //scale = 1;
      std::cout<<" WB coefficients (1): "<<wb_red_in<<"*"<<wb_red_mul<<" -> "<<wb_red_out<<std::endl
							 <<"                      "<<wb_green_in<<"*"<<wb_green_mul<<" -> "<<wb_green_out<<std::endl
							 <<"                      "<<wb_blue_in<<"*"<<wb_blue_mul<<" -> "<<wb_blue_out<<std::endl;
      std::cout<<"  scale: "<<scale<<std::endl;
      //float scale = wb_green_mul;
      wb_red_out /= scale;
      wb_green_out /= scale;
      wb_blue_out /= scale;
      wb_red_prop->update( wb_red_out );
      wb_green_prop->update( wb_green_out );
      wb_blue_prop->update( wb_blue_out );

      std::cout<<" WB coefficients (2): "<<wb_red_in<<"*"<<wb_red_mul<<" -> "<<wb_red_out<<std::endl
							 <<"                      "<<wb_green_in<<"*"<<wb_green_mul<<" -> "<<wb_green_out<<std::endl
							 <<"                      "<<wb_blue_in<<"*"<<wb_blue_mul<<" -> "<<wb_blue_out<<std::endl;

      wbRedSlider.init();
      wbGreenSlider.init();
      wbBlueSlider.init();

      img->update( pipeline, true );
      img->unlock();
    }

    std::cout<<"RawDeveloperConfigDialog: checking spot WB"<<std::endl;
		img->sample( l->get_id(), x, y, 7, NULL, values );
		if( values.size() != 3 ) {
			std::cout<<"RawDeveloperConfigDialog::pointer_relese_event(): values.size() "
							 <<values.size()<<" (!= 3)"<<std::endl;
			return;
		}
		rgb_check[0] = values[0];
		rgb_check[1] = values[1];
		rgb_check[2] = values[2];

    std::cout<<" rgb check("<<i<<"): "<<rgb_check[0]<<" "<<rgb_check[1]<<" "<<rgb_check[2]<<std::endl;

    if( i == 0 ) continue;
    float delta_r = rgb_check[0] - rgb_prev[1];
    float delta_g = rgb_check[1] - rgb_prev[1];
    float delta_b = rgb_check[2] - rgb_prev[2];
    if( (fabs(delta_r) < 0.001) && (fabs(delta_g) < 0.001) && (fabs(delta_b) < 0.001) )
      break;
    rgb_prev[0] = rgb_check[0];
    rgb_prev[1] = rgb_check[1];
    rgb_prev[2] = rgb_check[2];
  }

	// Update the prepipeline to reflect the new settings
	img->update();
}



void PF::RawDeveloperConfigDialog::color_spot_wb( double x, double y )
{
  // Get the layer associated to this operation
  PF::Layer* l = get_layer();
  if( !l ) return;

  // Get the image the layer belongs to
  PF::Image* img = l->get_image();
  if( !img ) return;
  
  // Get the default pipeline of the image 
  // (it is supposed to be at 1:1 zoom level 
  // and same accuracy as the preview one)
  PF::Pipeline* pipeline = img->get_pipeline( 0 );
  if( !pipeline ) return;

	// Make sure the first pipeline is up-to-date
	//img->update( pipeline, true );
	//img->update( NULL, true );
  //img->unlock();

  // Get the node associated to the layer
  PF::PipelineNode* node = pipeline->get_node( l->get_id() );
  if( !node ) return;

  // Finally, get the underlying VIPS image associated to the layer
  VipsImage* image = node->image;
  if( !image ) return;

  PropertyBase* wb_red_prop = wbRedSlider.get_prop();
  PropertyBase* wb_green_prop = wbGreenSlider.get_prop();
  PropertyBase* wb_blue_prop = wbBlueSlider.get_prop();
  if( !wb_red_prop || !wb_green_prop || !wb_blue_prop ) 
    return;

  // We need to retrieve the input ICC profile for the Lab conversion later on
  void *data;
  size_t data_length;
  if( vips_image_get_blob( image, VIPS_META_ICC_NAME, 
			   &data, &data_length ) )
    return;

  cmsHPROFILE profile_in = cmsOpenProfileFromMem( data, data_length );
  if( !profile_in ) 
    return;
  
  //#ifndef NDEBUG
  char tstr2[1024];
  cmsGetProfileInfoASCII(profile_in, cmsInfoDescription, "en", "US", tstr2, 1024);
  std::cout<<"raw_developer: embedded profile found: "<<tstr2<<std::endl;
  //#endif

  cmsCIExyY white;
  cmsWhitePointFromTemp( &white, 6500 );
  //cmsHPROFILE profile_out = cmsCreateLab4Profile( &white );
  cmsHPROFILE profile_out = cmsCreateLab4Profile( NULL );

  cmsUInt32Number infmt = TYPE_RGB_FLT;
  cmsUInt32Number outfmt = TYPE_Lab_FLT;

  cmsHTRANSFORM transform = cmsCreateTransform( profile_in, 
						infmt,
						profile_out, 
						outfmt,
						INTENT_PERCEPTUAL, cmsFLAGS_NOCACHE );
  if( !transform )
    return;

  cmsHTRANSFORM transform_inv = cmsCreateTransform( profile_out, 
						    outfmt,
						    profile_in, 
						    infmt,
						    INTENT_PERCEPTUAL, cmsFLAGS_NOCACHE );
  if( !transform_inv )
    return;

  //x = 2800; y = 654;

  PF::raw_preproc_sample_x = x;
  PF::raw_preproc_sample_y = y;

  
  float wb_red_mul = 1;
  float wb_green_mul = 1;
  float wb_blue_mul = 1;
  float wb_red_mul_prev = 1;
  float wb_green_mul_prev = 1;
  float wb_blue_mul_prev = 1;

  float Lab_check[3] = { 0, 0, 0 };
  float Lab_prev[3] = { 0, 1000, 1000 };
  for( int i = 0; i < 100; i++ ) {
    // Now we have to process a small portion of the image 
    // to get the corresponding Lab values
    VipsImage* spot;
    int left = (int)x-3;
    int top = (int)y-3;
    int width = 7;
    int height = 7;

    float wb_red_in;
    float wb_green_in;
    float wb_blue_in;
    wb_red_prop->get( wb_red_in );
    wb_green_prop->get( wb_green_in );
    wb_blue_prop->get( wb_blue_in );
    float norm_in = MIN3(wb_red_in,wb_green_in,wb_blue_in);
    //wb_red_in /= norm_in;
    //wb_green_in /= norm_in;
    //wb_blue_in /= norm_in;

    float wb_red_out;
    float wb_green_out;
    float wb_blue_out;

		/*
    VipsRect crop = {left, top, width, height};
    VipsRect all = {0 ,0, image->Xsize, image->Ysize};
    VipsRect clipped;
    vips_rect_intersectrect( &crop, &all, &clipped );
  
    if( vips_crop( image, &spot, 
									 clipped.left, clipped.top, 
									 clipped.width, clipped.height, 
									 NULL ) )
      return;

    VipsRect rspot = {0 ,0, spot->Xsize, spot->Ysize};

    VipsImage* outimg = im_open( "spot_wb_img", "p" );
    if (vips_sink_screen (spot, outimg, NULL,
													64, 64, 1, 
													0, NULL, this))
      return;
    VipsRegion* region = vips_region_new( outimg );
    if (vips_region_prepare (region, &rspot))
      return;
		*/
    //if( vips_sink_memory( spot ) )
    //  return;

    int sample_size = 15;
    int row, col;
    float* p;
    float red, green, blue;
    float rgb_avg[3] = {0, 0, 0};
		std::vector<float> values;

    std::cout<<std::endl<<std::endl<<"==============================================="<<std::endl;
   std::cout<<"RawDeveloperConfigDialog: getting color spot WB"<<std::endl;
		/*
    int line_size = clipped.width*3;
    for( row = 0; row < rspot.height; row++ ) {
      p = (float*)VIPS_REGION_ADDR( region, rspot.left, rspot.top );
      for( col = 0; col < line_size; col += 3 ) {
				red = p[col];      rgb_avg[0] += red;
				green = p[col+1];  rgb_avg[1] += green;
				blue = p[col+2];   rgb_avg[2] += blue;
				//std::cout<<"  pixel="<<row<<","<<col<<"    red="<<red<<"  green="<<green<<"  blue="<<blue<<std::endl;
      }
    }
    rgb_avg[0] /= rspot.width*rspot.height;
    rgb_avg[1] /= rspot.width*rspot.height;
    rgb_avg[2] /= rspot.width*rspot.height;
		*/
    std::cout<<"RawDeveloperConfigDialog: getting color spot WB ("<<x<<","<<y<<")"<<std::endl;
		img->sample( l->get_id(), x, y, sample_size, NULL, values );
		//values.clear(); img->sample( l->get_id(), x, y, sample_size, NULL, values );
		if( values.size() != 3 ) {
			std::cout<<"RawDeveloperConfigDialog::pointer_relese_event(): values.size() "
							 <<values.size()<<" (!= 3)"<<std::endl;
			return;
		}
		rgb_avg[0] = values[0];
		rgb_avg[1] = values[1];
		rgb_avg[2] = values[2];

    std::cout<<" RGB in: "<<rgb_avg[0]*255<<" "<<rgb_avg[1]*255<<" "<<rgb_avg[2]*255<<std::endl;

    float rgb_out[3] = {0, 0, 0};
    float Lab_in[3] = {0, 0, 0};
    float Lab_out[3] = {0, 0, 0};
    float Lab_wb[3] = {
      static_cast<float>(wb_target_L_slider.get_adjustment()->get_value()),
      static_cast<float>(wb_target_a_slider.get_adjustment()->get_value()),
      static_cast<float>(wb_target_b_slider.get_adjustment()->get_value())
    };
    //float Lab_wb[3] = {70, 15, 10};
    // Now we convert the average RGB values in the WB spot region to Lab
    cmsDoTransform( transform, rgb_avg, Lab_in, 1 );

    std::cout<<" Lab in: "<<Lab_in[0]<<" "<<Lab_in[1]<<" "<<Lab_in[2]<<std::endl;
    //return;

    const float epsilon = 1.0e-5;
    float ab_zero = 0;
    //float ab_zero = 0.5;
    float delta1 = Lab_in[1] - ab_zero;
    float delta2 = Lab_in[2] - ab_zero;

    float wb_delta1 = Lab_wb[1] - ab_zero;
    float wb_delta2 = Lab_wb[2] - ab_zero;

    if( (fabs(wb_delta1) < epsilon) &&
				(fabs(wb_delta2) < epsilon) ) {

      // The target color is gray, so we simply neutralize the spot value
      // The green channel is kept fixed and the other two are scaled to 
      // the green value
      wb_red_mul = rgb_avg[1]/rgb_avg[0];
      wb_blue_mul = rgb_avg[1]/rgb_avg[2];
      wb_green_mul = 1;

    } else if( fabs(wb_delta1) < epsilon ) {

      // The target "a" channel is very close to the neutral value,
      // in this case we set the ouput "a" channel equal to the target one
      // and we eventually invert the "b" channel if the input sign is opposite
      // to the target one, without applying any scaling
      Lab_out[0] = Lab_in[0];
      Lab_out[1] = Lab_wb[1];
      Lab_out[2] = Lab_in[2];
      if( delta2*wb_delta2 < 0 )
				Lab_out[2] = -Lab_in[2];

      // Now we convert back to RGB and we compute the multiplicative
      // factors that bring from the current WB to the target one
      cmsDoTransform( transform_inv, Lab_out, rgb_out, 1 );
      wb_red_mul = rgb_out[0]/rgb_avg[0];
      wb_green_mul = rgb_out[1]/rgb_avg[1];
      wb_blue_mul = rgb_out[2]/rgb_avg[2];

    } else if( fabs(wb_delta2) < epsilon ) {

      // The target "b" channel is very close to the neutral value,
      // in this case we set the ouput "b" channel equal to the target one
      // and we eventually invert the "a" channel if the input sign is opposite
      // to the target one, without applying any scaling
      Lab_out[0] = Lab_in[0];
      Lab_out[1] = Lab_in[1];
      Lab_out[2] = Lab_wb[2];
      if( delta1*wb_delta1 < 0 )
				Lab_out[1] = -Lab_in[1];

      // Now we convert back to RGB and we compute the multiplicative
      // factors that bring from the current WB to the target one
      cmsDoTransform( transform_inv, Lab_out, rgb_out, 1 );
      wb_red_mul = rgb_out[0]/rgb_avg[0];
      wb_green_mul = rgb_out[1]/rgb_avg[1];
      wb_blue_mul = rgb_out[2]/rgb_avg[2];

    } else {

      // Both "a" and "b" target channels are different from zero, so we try to 
      // preserve the target a/b ratio
      float sign1 = (delta1*wb_delta1 < 0) ? -1 : 1;
      float sign2 = (delta2*wb_delta2 < 0) ? -1 : 1;
      float ab_ratio = (sign1*delta1)/(sign2*delta2);
      float wb_ab_ratio = wb_delta1/wb_delta2;

      Lab_out[0] = Lab_in[0];
      if( fabs(wb_delta1) > fabs(wb_delta2) ) {
				Lab_out[1] = sign1*delta1 + ab_zero;
				Lab_out[2] = sign2*delta2*ab_ratio/wb_ab_ratio + ab_zero;
      } else {
				Lab_out[1] = sign1*delta1*wb_ab_ratio/ab_ratio + ab_zero;
				Lab_out[2] = sign2*delta2 + ab_zero;
      }
      Lab_out[1] = Lab_wb[1];
      Lab_out[2] = Lab_wb[2];
      std::cout<<" Lab out: "<<Lab_out[0]<<" "<<Lab_out[1]<<" "<<Lab_out[2]<<std::endl;

      float delta_a = Lab_out[1] - Lab_in[1];
      float delta_b = Lab_out[2] - Lab_in[2];
      float wb_red_out1 = wb_red_in;
      float wb_red_out2 = wb_red_in;
      float wb_green_out1 = wb_green_in;
      float wb_green_out2 = wb_green_in;
      float wb_blue_out1 = wb_blue_in;
      float wb_blue_out2 = wb_blue_in;

      if( Lab_out[1] >= 0 ) {
        // Target "a" is positive, therefore we have to act 
        // on the red and blue multipliers simultaneously
        wb_red_out1 += wb_red_in * (delta_a*0.1/Lab_out[1]);
        wb_blue_out1 += wb_blue_in * (delta_a*0.1/Lab_out[1]);
      } else {
        // Target "a" is negative, therefore we have to act 
        // on the green channel only
        wb_green_out1 += wb_green_in * (delta_a*0.1/Lab_out[1]);
      }

      if( Lab_out[2] >= 0 ) {
        // Target "b" is positive, therefore we have to act 
        // on the red and green multipliers simultaneously
        wb_red_out2 += wb_red_in * (delta_b*0.1/Lab_out[2]);
        wb_green_out2 += wb_green_in * (delta_b*0.1/Lab_out[2]);
      } else {
        // Target "b" is negative, therefore we have to act 
        // on the blue channel only
        wb_blue_out2 += wb_blue_in * (delta_b*0.1/Lab_out[2]);
      }

      wb_red_out = (wb_red_out1 + wb_red_out2)/2.0f;
      wb_green_out = (wb_green_out1 + wb_green_out2)/2.0f;
      wb_blue_out = (wb_blue_out1 + wb_blue_out2)/2.0f;

      /*
        // Now we convert back to RGB and we compute the multiplicative
        // factors that bring from the current WB to the target one
        cmsDoTransform( transform_inv, Lab_out, rgb_out, 1 );
        std::cout<<" RGB out: "<<rgb_out[0]*255<<" "<<rgb_out[1]*255<<" "<<rgb_out[2]*255<<std::endl;
        
        wb_red_mul = rgb_out[0]/rgb_avg[0];
        wb_green_mul = rgb_out[1]/rgb_avg[1];
        wb_blue_mul = rgb_out[2]/rgb_avg[2];
        
        float f = 1.5;
        wb_red_out = (f*wb_red_mul+1-f)*wb_red_in;
        wb_green_out = (f*wb_green_mul+1-f)*wb_green_in;
        wb_blue_out = (f*wb_blue_mul+1-f)*wb_blue_in;
        float scale = (wb_red_out+wb_green_out+wb_blue_out)/3.0f;
        std::cout<<" scale: "<<scale<<std::endl;
      //float norm_out = MIN3(wb_red_out,wb_green_out,wb_blue_out);
      */
      /*
      // Scale target L channel according to norm_out
      Lab_out[0] /= norm_out*1.01;
      std::cout<<" Lab out #2: "<<Lab_out[0]<<" "<<Lab_out[1]<<" "<<Lab_out[2]<<std::endl;

      // Repeat the transform with the new luminosity
      cmsDoTransform( transform_inv, Lab_out, rgb_out, 1 );
      std::cout<<" RGB out #2: "<<rgb_out[0]*255<<" "<<rgb_out[1]*255<<" "<<rgb_out[2]*255<<std::endl;

      wb_red_mul = rgb_out[0]/rgb_avg[0];
      wb_green_mul = rgb_out[1]/rgb_avg[1];
      wb_blue_mul = rgb_out[2]/rgb_avg[2];
    
      wb_red_out = wb_red_mul*wb_red_in;
      wb_green_out = wb_green_mul*wb_green_in;
      wb_blue_out = wb_blue_mul*wb_blue_in;
      */
    }

    /*
    float wb_min = MIN3( wb_red_mul, wb_green_mul, wb_blue_mul );
    wb_red_mul /= wb_min;
    wb_green_mul /= wb_min;
    wb_blue_mul /= wb_min;

    float wb_red_d = wb_red_mul - wb_red_mul_prev;
    float wb_green_d = wb_green_mul - wb_green_mul_prev;
    float wb_blue_d = wb_blue_mul - wb_blue_mul_prev;

    wb_red_mul = wb_red_mul_prev + 1.00001*wb_red_d;
    wb_green_mul = wb_green_mul_prev + 1.00001*wb_green_d;
    wb_blue_mul = wb_blue_mul_prev + 1.00001*wb_blue_d;

    wb_red_mul_prev = wb_red_mul;
    wb_green_mul_prev = wb_green_mul;
    wb_blue_mul_prev = wb_blue_mul;
    */
    /*
    // The WB multiplicative factors are scaled so that their product is equal to 1
    float scale = wb_red_mul*wb_green_mul*wb_blue_mul;
    //float scale = wb_green_mul;
    wb_red_mul /= scale;
    wb_green_mul /= scale;
    wb_blue_mul /= scale;
    */

    //float wb_red_out = wb_red_mul*wb_red_in;
    // float wb_green_out = wb_green_mul*wb_green_in;
    //float wb_blue_out = wb_blue_mul*wb_blue_in;
    //float scale = (wb_red_out+wb_green_out+wb_blue_out)/3.0f;
    //float scale = MIN3(wb_red_out,wb_green_out,wb_blue_out);
    float scale = MIN3(wb_red_out,wb_green_out,wb_blue_out);
    //scale = 1;
    std::cout<<" WB coefficients (1): "<<wb_red_in<<"*"<<wb_red_mul<<" -> "<<wb_red_out<<std::endl
             <<"                      "<<wb_green_in<<"*"<<wb_green_mul<<" -> "<<wb_green_out<<std::endl
             <<"                      "<<wb_blue_in<<"*"<<wb_blue_mul<<" -> "<<wb_blue_out<<std::endl;
    std::cout<<"  scale: "<<scale<<std::endl;
    //float scale = wb_green_mul;
    wb_red_out /= scale;
    wb_green_out /= scale;
    wb_blue_out /= scale;
    wb_red_prop->update( wb_red_out );
    wb_green_prop->update( wb_green_out );
    wb_blue_prop->update( wb_blue_out );

    std::cout<<" WB coefficients (2): "<<wb_red_in<<"*"<<wb_red_mul<<" -> "<<wb_red_out<<std::endl
             <<"                      "<<wb_green_in<<"*"<<wb_green_mul<<" -> "<<wb_green_out<<std::endl
             <<"                      "<<wb_blue_in<<"*"<<wb_blue_mul<<" -> "<<wb_blue_out<<std::endl;

    wbRedSlider.init();
    wbGreenSlider.init();
    wbBlueSlider.init();

    //bool async = img->is_async();
    //img->set_async( false );
    //img->update( pipeline, true );
    img->update( NULL, true );
    img->unlock();
    //img->set_async( async );


		/*
    g_object_unref( spot );
    g_object_unref( outimg );
    g_object_unref( region );

    if( vips_crop( image, &spot, 
									 clipped.left, clipped.top, 
									 clipped.width, clipped.height, 
									 NULL ) )
      return;

    outimg = im_open( "spot_wb_img", "p" );
    if (vips_sink_screen (spot, outimg, NULL,
													64, 64, 1, 
													0, NULL, this))
      return;
    region = vips_region_new( outimg );
    if (vips_region_prepare (region, &rspot))
      return;
  
    std::cout<<"RawDeveloperConfigDialog: checking spot WB"<<std::endl;
    rgb_avg[0] = rgb_avg[1] = rgb_avg[2] = 0;
    for( row = 0; row < rspot.height; row++ ) {
      p = (float*)VIPS_REGION_ADDR( region, rspot.left, rspot.top );
      for( col = 0; col < line_size; col += 3 ) {
				red = p[col];      rgb_avg[0] += red;
				green = p[col+1];  rgb_avg[1] += green;
				blue = p[col+2];   rgb_avg[2] += blue;
				//std::cout<<"  pixel="<<row<<","<<col<<"    red="<<red<<"  green="<<green<<"  blue="<<blue<<std::endl;
      }
    }
    rgb_avg[0] /= rspot.width*rspot.height;
    rgb_avg[1] /= rspot.width*rspot.height;
    rgb_avg[2] /= rspot.width*rspot.height;
		*/
    std::cout<<"RawDeveloperConfigDialog: checking spot WB"<<std::endl;
		img->sample( l->get_id(), x, y, sample_size, NULL, values );
		if( values.size() != 3 ) {
			std::cout<<"RawDeveloperConfigDialog::pointer_relese_event(): values.size() "
							 <<values.size()<<" (!= 3)"<<std::endl;
			return;
		}
		rgb_avg[0] = values[0];
		rgb_avg[1] = values[1];
		rgb_avg[2] = values[2];

    std::cout<<" RGB check: "<<rgb_avg[0]*255<<" "<<rgb_avg[1]*255<<" "<<rgb_avg[2]*255<<std::endl;
    // Now we convert the average RGB values in the WB spot region to Lab
    cmsDoTransform( transform, rgb_avg, Lab_check, 1 );
    std::cout<<" Lab check("<<i<<"): "<<Lab_check[0]<<" "<<Lab_check[1]<<" "<<Lab_check[2]<<std::endl;

		/*
    g_object_unref( spot );
    g_object_unref( outimg );
    g_object_unref( region );
		*/

    if( i == 0 ) continue;
    float delta_a = Lab_check[1] - Lab_prev[1];
    float delta_b = Lab_check[2] - Lab_prev[2];
    if( (fabs(delta_a) < 0.005) && (fabs(delta_b) < 0.005) )
      break;
    Lab_prev[0] = Lab_check[0];
    Lab_prev[1] = Lab_check[1];
    Lab_prev[2] = Lab_check[2];
  }

  PF::raw_preproc_sample_x = 0;
  PF::raw_preproc_sample_y = 0;

  char tstr[500];
  snprintf( tstr, 499, "Best match: L=%0.2f a=%0.2f b=%0.2f",
						Lab_check[0], Lab_check[1], Lab_check[2] );
  wb_best_match_label.set_text( tstr );

  cmsDeleteTransform( transform );
  cmsDeleteTransform( transform_inv );
  cmsCloseProfile( profile_in );
  cmsCloseProfile( profile_out );

	// Update the preview to reflect the new settings
	img->update();
}



void PF::RawDeveloperConfigDialog::on_cam_button_open_clicked()
{
  Gtk::FileChooserDialog dialog("Please choose a file",
																Gtk::FILE_CHOOSER_ACTION_OPEN);
  dialog.set_transient_for(*this);
  
  //Add response buttons the the dialog:
  dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
  dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);

  //Show the dialog and wait for a user response:
  int result = dialog.run();

  //Handle the response:
  switch(result) {
  case(Gtk::RESPONSE_OK): 
    {
      std::cout << "Open clicked." << std::endl;

      //Notice that this is a std::string, not a Glib::ustring.
      std::string filename = dialog.get_filename();
      std::cout << "File selected: " <<  filename << std::endl;
      camProfFileEntry.set_text( filename.c_str() );
      on_cam_filename_changed();
      break;
    }
  case(Gtk::RESPONSE_CANCEL): 
    {
      std::cout << "Cancel clicked." << std::endl;
      break;
    }
  default: 
    {
      std::cout << "Unexpected button clicked." << std::endl;
      break;
    }
  }
}



void PF::RawDeveloperConfigDialog::on_out_button_open_clicked()
{
  Gtk::FileChooserDialog dialog("Please choose a file",
																Gtk::FILE_CHOOSER_ACTION_OPEN);
  dialog.set_transient_for(*this);
  
  //Add response buttons the the dialog:
  dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
  dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);

  //Show the dialog and wait for a user response:
  int result = dialog.run();

  //Handle the response:
  switch(result) {
  case(Gtk::RESPONSE_OK): 
    {
      std::cout << "Open clicked." << std::endl;

      //Notice that this is a std::string, not a Glib::ustring.
      std::string filename = dialog.get_filename();
      std::cout << "File selected: " <<  filename << std::endl;
      outProfFileEntry.set_text( filename.c_str() );
      on_out_filename_changed();
      break;
    }
  case(Gtk::RESPONSE_CANCEL): 
    {
      std::cout << "Cancel clicked." << std::endl;
      break;
    }
  default: 
    {
      std::cout << "Unexpected button clicked." << std::endl;
      break;
    }
  }
}



void PF::RawDeveloperConfigDialog::on_cam_filename_changed()
{
  if( get_layer() && get_layer()->get_image() && 
      get_layer()->get_processor() &&
      get_layer()->get_processor()->get_par() ) {
    std::string filename = camProfFileEntry.get_text();
    if( filename.empty() )
      return;
    std::cout<<"New input profile name: "<<filename<<std::endl;
    PF::RawDeveloperPar* par = 
      dynamic_cast<PF::RawDeveloperPar*>(get_layer()->get_processor()->get_par());
    if( !par )
      return;
    PropertyBase* prop = par->get_property( "cam_profile_name" );
    if( !prop ) 
      return;
    prop->update( filename );
    get_layer()->set_dirty( true );
    std::cout<<"  updating image"<<std::endl;
    get_layer()->get_image()->update();
  }
}



void PF::RawDeveloperConfigDialog::on_out_filename_changed()
{
  if( get_layer() && get_layer()->get_image() && 
      get_layer()->get_processor() &&
      get_layer()->get_processor()->get_par() ) {
    std::string filename = outProfFileEntry.get_text();
    if( filename.empty() )
      return;
    std::cout<<"New output profile name: "<<filename<<std::endl;
    PF::RawDeveloperPar* par = 
      dynamic_cast<PF::RawDeveloperPar*>(get_layer()->get_processor()->get_par());
    if( !par )
      return;
    PropertyBase* prop = par->get_property( "out_profile_name" );
    if( !prop ) 
      return;
    prop->update( filename );
    get_layer()->set_dirty( true );
    std::cout<<"  updating image"<<std::endl;
    get_layer()->get_image()->update();
  }
}
