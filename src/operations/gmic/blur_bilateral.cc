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


#include "gmic.hh"
#include "blur_bilateral.hh"



PF::GmicBlurBilateralPar::GmicBlurBilateralPar(): 
  OpParBase(),
  iterations("iterations",this,1),
  sigma_s("sigma_s",this,10),
  sigma_r("sigma_r",this,7),
  bgrid_s("bgrid_s",this,-33), 
  bgrid_r("bgrid_r",this,32)
{	
  gmic = PF::new_gmic();
  set_type( "gmic_blur_bilateral" );
}



VipsImage* PF::GmicBlurBilateralPar::build(std::vector<VipsImage*>& in, int first, 
				   VipsImage* imap, VipsImage* omap, 
				   unsigned int& level)
{
  VipsImage* srcimg = NULL;
  if( in.size() > 0 ) srcimg = in[0];
  VipsImage* mask;
  VipsImage* out = srcimg;

  if( !out ) return NULL;
  
  if( !(gmic->get_par()) ) return NULL;
  PF::GMicPar* gpar = dynamic_cast<PF::GMicPar*>( gmic->get_par() );
  if( !gpar ) return NULL;

  float ss = sigma_s.get(), sr = sigma_r.get();
	for( int l = 1; l <= level; l++ ) {
		ss /= 2; sr /= 2;
  }
  char command[500];
  snprintf(command,499,"-bilateral %f,%f", ss, sr);
  gpar->set_command( command );
  gpar->set_iterations( iterations.get() );
  gpar->set_padding( (int)( MAX(ss,sr)*1.5 ) );
  gpar->set_x_scale( 1.0f );
  gpar->set_y_scale( 1.0f );
  /*
	if( (get_render_mode() == PF_RENDER_PREVIEW && level>0) ) {
		PF_REF( out, "PF::CimgBlurBilateralPar::build(): out ref" );
		return out;
	}
  */

  gpar->set_image_hints( srcimg );
  gpar->set_format( get_format() );

  out = gpar->build( in, first, imap, omap, level );
  if( !out ) {
    std::cout<<"gmic.build() failed!!!!!!!"<<std::endl;
  }

	return out;
}


PF::ProcessorBase* PF::new_gmic_blur_bilateral()
{
  return( new PF::Processor<PF::GmicBlurBilateralPar,PF::GmicBlurBilateralProc>() );
}
