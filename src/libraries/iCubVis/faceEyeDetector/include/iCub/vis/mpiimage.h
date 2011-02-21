/*! \file mpiimage.h
*   Header for Machine Perception lab Integral Image container object
*
*   Created by Ian Fasel on Feb 02, 2003.
* 
*   Copyright (c) 2003 Machine Perception Laboratory 
*   University of California San Diego.
* 
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
*
*    1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
*    2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
*    3. The name of the author may not be used to endorse or promote products derived from this software without specific prior written permission.
* 
* THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* 
* Three main goals:
*    -# Simplify interface for iterating across all windows at all scales
*    -# Provide easy region of interest (ROI) operator
*    -# allow use of cached indexes for fast pixel lookups
*
* Usage:
* MPIImagePyramid ip(image_list, scale_factor, ROI, stride, window_size);
* for(MPIImagePyramid::iterator scale = ip.begin(); scale != ip.end(); ++scale){
*    for(MPIImagePyramid::MPIScaledImage::iterator iter = (*scale).begin(); iter != (*scale).end(); ++iter){
*       iter.getPixel(0, ind);
*       iter.getPixel(1, ind);
*    }
* }
*/

#ifndef _MPIIMAGE_H_
#define _MPIIMAGE_H_

#include <fstream>
#include <vector>

#include "rimage.h"
#include "square.h"
#include "roi.h"
#include "faceboxlist.h"

#ifdef WIN32
#include <windows.h>
#endif


using namespace std;

template<class T> class MPIImagePyramid; // forward declaration for convenience

/*!
\brief The elements of an MPIImagePyramid, a container class who's iterator
 steps through all the subwindows in the image at the given scale.
 
 Generated by dereferencing an MPIImagePyramid<T>::iterator, conceptually it 
 is the container for all of the subwindows of the image at a particular scale in the 
 image pyramid.  Typically, one iterates through each window and performs some
 computation using the pixels in that window.  If the results are stored into another
 image, then this can be conceptualized as a convolution.
 */
template<class T>
class MPIScaledImage{
  //private:
  
 public:
  const MPIImagePyramid<T>& m_ref;
  float shift;
  const int m_scale_ind;
  const int m_scale_factor;
  const float m_true_scale_factor;
  int   subwin_size_x; // Note: this is pure evil if m_true_scale_factor * (m_ref.m_window_x) is not an integer;
  int   subwin_size_y; // same as above
  int m_min_x, m_max_x, m_min_y, m_max_y;
  MPIScaledImage(const MPIImagePyramid<T>& ref, const int &scale_ind)
    : m_ref(ref), m_scale_ind(scale_ind), m_scale_factor(static_cast<int>(m_ref.scale_factors[scale_ind]+0.5)),
      m_true_scale_factor(ref.scale_factors[scale_ind]) {
    shift = max(1.0f,m_scale_factor*(m_ref.m_stride + 0.00001f));  // Fudge to make sure floor works correctly
    subwin_size_x = static_cast<int>(m_true_scale_factor * (m_ref.m_window_x)+0.0001f);
    subwin_size_y = static_cast<int>(m_true_scale_factor * (m_ref.m_window_y)+0.0001f);
    m_min_x = ref.m_roi.vmin_x[scale_ind];
    m_max_x = ref.m_roi.vmax_x[scale_ind];
    m_min_y = ref.m_roi.vmin_y[scale_ind];
    m_max_y = ref.m_roi.vmax_y[scale_ind];
  }
  /*! \brief Pointer to a window at container's scale
    
    This iterator allows pixels to be accessed in a manner independent of the actual
    offset and scale of the window it points to.  Thus, accessing e.g., pixel 4,3 of
    the iterator will provide pixel 4,3 in the referenced image adjusted with respect to 
    the offset and scale of the window iterator.
    
    Asking for pixel x,y requires some addition and multiplication operations that can
    be avoided if a relative index is provided directly.  This is simple and fast to
    do -- since you know the size of the image, the relative offset from 0,0 can be
    computed once and then reused for all the windows in the scale.
    
    This iterator can be used to access any layer of the image list if there
    is more than one image in the base image list.  A few specialized functions exist which
    assume there are at least two images in the list and allow an additional speedup for
    pixel access in these top two images.
  */
  class const_iterator{
  private:
    typedef const_iterator self;
    const MPIScaledImage< T > & m_ref;
    const vector< RImage< T > * >& ref_image; //  gcc 2.95 requires this... bug in gcc?
    const T* pixel0, *pixel1; // a reference to the first pixel of the first two images. Bad if there aren't enough windows.
    int m_max_x, m_max_y, m_max_valid_x_ind, m_max_valid_y_ind;
    float m_pos_x, m_pos_y;
    int m_working_x, m_working_y;
    //int old_x, old_y;
    int m_ind, m_line_ind;
    int m_min_x;
    float m_min_x_f;
    float m_shift;
    public:
      const_iterator(MPIScaledImage<T>& ref, const int &pos_x, const int &pos_y) : m_ref(ref),
	ref_image(m_ref.m_ref.m_image), m_shift(ref.shift) {
            m_working_x = pos_x;
            m_working_y = pos_y;
            m_pos_x = static_cast<float>(m_working_x);
            m_pos_y = static_cast<float>(m_working_y);
            m_line_ind = m_working_y*ref_image[0]->width;
            m_ind = m_line_ind + m_working_x;
            m_max_x = m_ref.m_max_x - (m_ref.subwin_size_x); 
            m_max_y = m_ref.m_max_y - (m_ref.subwin_size_y);
            m_max_valid_x_ind = ref_image[0]->width - m_ref.subwin_size_x;
            m_max_valid_y_ind = ref_image[0]->height - m_ref.subwin_size_y;
            m_min_x = m_ref.m_min_x;
            m_min_x_f = static_cast<float>(m_min_x);

        pixel0 = ref_image[0]->array + m_ind;
        pixel1 = ref_image[1]->array + m_ind;
 		}
		
      // Increment Logic
		inline void do_increment(){
		  m_pos_x += m_shift;
		  m_working_x = static_cast<int>(m_pos_x);
		  if(m_working_x < m_max_x){
		    m_ind = m_line_ind + m_working_x;
		  } else {
            m_pos_x = m_min_x_f; 
		    m_working_x = m_min_x; 
		    m_pos_y += m_shift;
		    m_working_y = static_cast<int>(m_pos_y);
		    m_line_ind = m_working_y*ref_image[0]->width;
		    m_ind = m_line_ind + m_working_x;
		  }
		  pixel0 = ref_image[0]->array + m_ind;
		  pixel1 = ref_image[1]->array + m_ind;
		}
		/// Preincrement
		inline self& operator++ () { 
		  do_increment(); 
		  return *this; }
		/// Postincrement
		inline self operator++ (int) { self tmp = *this; do_increment(); return tmp; }
		inline self& operator=(const self &it){
		  m_max_x = it.m_max_x; m_max_y=it.m_max_y; m_max_valid_x_ind=it.m_max_valid_x_ind;
		  m_max_valid_y_ind=it.m_max_valid_y_ind; m_pos_x=it.m_pos_x; m_pos_y=it.m_pos_y;
		  m_working_x=it.m_working_x; m_working_y=it.m_working_y; m_ind=it.m_ind; m_line_ind=it.m_line_ind;
		  return *this;
		}
		//: Predecrement
		//inline self& operator-- () { --m_pos; return *this; }
		//: Postdecrement
		//inline self operator-- (int) { self tmp = *this; --m_pos; return tmp; }
		
		inline T operator*() const{ return ref_image[0]->getPixel( m_ind ); }
		inline bool operator!=(const self& x) const { return m_ind != x.m_ind; }
		inline bool operator<=(const self& x) const { return m_pos_y <= x.m_pos_y; }
		inline RImage<T>& operator[](int i) const { return *ref_image[i]; }
		inline void getIndex(int &ind) const { ind = m_ind; }
		inline void getCoords(int &x, int &y) const { x = m_working_x, y = m_working_y; }
		inline int getSize() const {return m_ref.subwin_size_x;}
		
		//-------------------------------------------
		// Get Pixel functions
		//
      //! Get the pixels in image i using index ind
      inline T getPixel(const int &i, const int &ind){
        return ref_image[i]->getPixel(m_ind+ind); }
      //! A get pixel function for getting at image[0] pixels really fast.
      inline T getPixel0(const int &ind) const { return pixel0[ind]; }
      //! A get pixel function for getting at image[1] pixels really fast.
      inline T getPixel1(const int &ind) const { return pixel1[ind]; }
      //! special getPixel for cases where it is necessary to protect against going off the edge of the image
      inline T getPixel(const int &i, const int &ind, const int &x, const int &y) const {
	  if((m_working_x < 0) || (m_working_y < 0) || (m_working_x > m_max_valid_x_ind) || (m_working_y > m_max_valid_y_ind))
	    return ref_image[i]->getPixel(m_working_x + x, m_working_y + y);
	  else
	    return ref_image[i]->getPixel(m_ind+ind);
      }
      inline T getScalePixel(const int &i, const int &x, const int &y) const {
        return ref_image[i]->getPixel(m_working_x + x * m_ref.m_scale_factor, m_working_y + y * m_ref.m_scale_factor); }

      //! get pixels that correspond to shifting the window (i.e., take stride into account)
      inline T getShiftPixel(const int &i, const int &x, const int &y){
        return ref_image[i]->getPixel(static_cast<int>(m_pos_x + x * m_shift),
                                      static_cast<int>(m_pos_y + y * m_shift)); }
      //--------------------------------------------


      //-------------------------------------------
      // Set Pixel functions
      //
      inline void setPixel(const int &i, const int &ii, const T &val){
			ref_image[i]->setPixel(m_ind+ii, val); }
      inline void setPixel(const int &i, const int &x, const int &y, const T &val){
            ref_image[i]->setPixel(static_cast<int>(m_pos_x + x),
                                   static_cast<int>(m_pos_y + y), val); }
      inline void setShiftPixel(const int &i, const int &x, const int &y, const T &val){
			ref_image[i]->setPixel(static_cast<int>(m_pos_x + x * m_shift),
				      static_cast<int>(m_pos_y + y * m_shift), val); }
      inline void setScalePixel(const int &i, const int &x, const int &y, const T &val){
			ref_image[i]->setPixel(m_working_x + x * m_ref.m_scale_factor,
				      m_working_y + y * m_ref.m_scale_factor, val); }
      // set pixels that correspond to shifting the window (i.e., take stride into account)
      //--------------------------------------------
      inline Square getSquare() const { return Square(m_ref.subwin_size_x, m_working_x, m_working_y, m_ref.m_scale_ind); }
    };
    friend class const_iterator;

    inline const_iterator begin(){ return const_iterator(*this, m_min_x, m_min_y); }
    inline const_iterator end(){
      // This is inefficient, but trying to figure it out correctly is very error-prone. Help!
      int working_y = m_min_y;
      float pos_y = static_cast<float>(working_y);
      while((working_y + subwin_size_y) < m_max_y){
	pos_y += shift;
	working_y = static_cast<int>(pos_y);
      }
      return const_iterator(*this, m_min_x, working_y);// - (m_scale_factor - 1));
    }
};


/*!
\brief A container class representing all the patches at all scales in an image.
 
 Typically one iterates through the scales, then iterates through all the
 patches at that scale
*/
template<class T>
class MPIImagePyramid{
  friend class MPIScaledImage<T>;
  // Issues annoying warning on gcc 3.2, but I think its a bug in gcc
  friend class MPIScaledImage<T>::const_iterator; 

  // Public Functions and data
 public:
  MPIImagePyramid(RImage<T> &image, float scale_factor, int window_x, int window_y, float stride)
    : m_stride(stride), m_scale_factor(scale_factor), 
    m_window_x(window_x), m_window_y(window_y){
    m_image.push_back(&image);
    SetScaleFactors();
    InitROI(); }
	 
  MPIImagePyramid(vector< RImage<T>* > &image, float scale_factor, int window_x, int window_y, float stride)
    : m_stride(stride), m_image(image), m_scale_factor(scale_factor),
    m_window_x(window_x), m_window_y(window_y){
    SetScaleFactors();
    InitROI(); } 

  ~MPIImagePyramid(){ }
	 
  class const_iterator{
  private:
    typedef const_iterator self;
    const MPIImagePyramid<T>& m_ref; 
    int m_pos;
  public:
    const_iterator(const MPIImagePyramid &ref, const int &pos) : m_ref(ref), m_pos(pos) {
      //;//cout << "scale iterator: m_pos="<<m_pos<<endl;
    }
    // Preincrement
    inline self& operator++ () { ++m_pos; return *this; }
    // Postincrement
    inline self operator++ (int) { self tmp = *this; ++m_pos; return tmp; }
    // Preincrement
    inline self& operator-- () { --m_pos; return *this; }
    // Postincrement
    inline self operator-- (int) { self tmp = *this; --m_pos; return tmp; }
    // Dereference
    MPIScaledImage<T> operator*() const{ return MPIScaledImage<T>(m_ref, m_pos);}
    // Notequals
    inline bool operator!=(const self& x) const { return m_pos != x.m_pos; }
    inline int getScale(float &sf){sf = m_ref.scale_factors[m_pos]; return m_pos; }
    inline float getScale(){return m_ref.scale_factors[m_pos];}
  };

  friend class const_iterator;
  inline const_iterator begin(){
    return const_iterator(*this, m_roi.m_min_scale); }
  inline const_iterator end(){
    return const_iterator(*this, m_roi.m_max_scale); }
  inline float getMaxScale(){return scale_factors[scale_factors.size() - 1];}
  inline int getMaxScale(float &sf){sf = scale_factors[scale_factors.size() - 1]; return scale_factors.size() - 1;}

  vector< float > scale_factors;
  float m_stride;
	 
 private:
  // Private data
  vector< RImage<T>* > m_image;
  ROI m_roi;
  float m_scale_factor;
  int m_window_x, m_window_y;
  float temp;
  
 private:
  // Private Functions
  inline int imax(const int &x, const int &y) const { return(x > y ? x : y);}
  inline int imin(const int &x, const int &y) const { return(x < y ? x : y);}
  inline int scale(const int &x, const int &y) const { return(x * y);}
  void SetScaleFactorsFloat(){
      // Find maximum scales
      float max_y_scale_factor = static_cast<int>(m_image[0]->height / (m_window_y+1));
      float max_x_scale_factor = static_cast<int>(m_image[0]->width / (m_window_x+1));
      if (max_x_scale_factor && max_y_scale_factor){
          float scale_factor = 1.0f;
          while((scale_factor < max_y_scale_factor) && (scale_factor < max_x_scale_factor)){
              scale_factors.push_back(scale_factor);
              temp = (float)(static_cast<int>(scale_factor*m_scale_factor*(float)m_window_x + .99999f))/(float)m_window_x;
              scale_factor = temp;
          }
          // make the last scale such that the window gets as close as possible to the full image
          scale_factor = min(max_x_scale_factor,max_y_scale_factor);
          scale_factors.push_back(scale_factor);
      }
  }
  void SetScaleFactors(){
    // Find maximum scales
    int max_y_scale_factor = static_cast<int>(m_image[0]->height / (m_window_y+1));
    int max_x_scale_factor = static_cast<int>(m_image[0]->width / (m_window_x+1));
    if (max_x_scale_factor && max_y_scale_factor){
      int scale_factor = 1;
      while((scale_factor < max_y_scale_factor) && (scale_factor < max_x_scale_factor)){
	scale_factors.push_back((float)scale_factor);
	temp = (float)imax (scale_factor+1, static_cast<int>(scale_factor*m_scale_factor));
	scale_factor = (int)temp;
      }
      // make the last scale such that the window gets as close as possible to the full image
      scale_factor = imin(max_x_scale_factor,max_y_scale_factor);
      scale_factors.push_back((const float)scale_factor);
    }
  }
  void SetScaleFactorsNew(int start, int scale_size, float percent){
    //Find maximum scales
    int max_y_scale_factor = static_cast<int>(m_image[0]->height / (m_window_y+1));
    int max_x_scale_factor = static_cast<int>(m_image[0]->width / (m_window_x+1));
    if (max_x_scale_factor && max_y_scale_factor){
      int max_scale_factor = imin(max_x_scale_factor, max_y_scale_factor);
      int current_scale = 0;
      int previous_scale = start;
      int previous_point, current_point;
      scale_factors.push_back(start);
      while(current_scale < max_scale_factor){
	current_scale = previous_scale+1;
	previous_point = scale(previous_scale, scale_size) + (scale(previous_scale, scale_size) * percent);
	while(current_scale < max_scale_factor){
	  current_point = scale(current_scale, scale_size) - (scale(current_scale, scale_size) * percent);
	  if(current_point < previous_point)
	    current_scale++;
	  else{
	    if(current_scale - 1 == previous_scale)
	      current_scale++;//to make sure next scale != to previous scale
	    scale_factors.push_back(current_scale - 1);
	    previous_scale = current_scale - 1;
	    break;
	  }
	}
      }
      // make the last scale such that the window gets as close as possible to the full image
      // if gap between previous_scale and max_scale then also add prior scale
      current_point = scale(max_scale_factor, scale_size) - (scale(max_scale_factor, scale_size) * percent);
      if(current_point > previous_point && (max_scale_factor - 1) > previous_scale)
	scale_factors.push_back(max_scale_factor -1);
      scale_factors.push_back(max_scale_factor);
    }
  }
  void InitROIvectors(){
    m_roi.vmin_x.clear();
    m_roi.vmax_x.clear();
    m_roi.vmin_y.clear();
    m_roi.vmax_y.clear();
    for(unsigned int i=0; i < scale_factors.size(); ++i){
      m_roi.vmin_x.push_back(m_roi.m_min_x);
      m_roi.vmax_x.push_back(m_roi.m_max_x);
      m_roi.vmin_y.push_back(m_roi.m_min_y);
      m_roi.vmax_y.push_back(m_roi.m_max_y);
    }
  }

 public:
  void InitROI(){
    m_roi.m_min_x = 0;
    m_roi.m_min_y = 0;
    m_roi.m_min_scale = 0;
    m_roi.m_max_x = m_image[0]->width;
    m_roi.m_max_y = m_image[0]->height;
    m_roi.m_max_scale = scale_factors.size();
    m_roi.m_limit_scale = scale_factors.size();
    InitROIvectors();
  }
  ROI SetROI(ROI &roi){
    // set ROI to boundaries of image and scales
    m_roi = roi;
    if(m_roi.m_min_x < 0) m_roi.m_min_x = 0;
    if(m_roi.m_max_x > m_image[0]->width) m_roi.m_max_x = m_image[0]->width;
    if(m_roi.m_min_y < 0) m_roi.m_min_y = 0;
    if(m_roi.m_max_y > m_image[0]->height) m_roi.m_max_y = m_image[0]->height;
    if(m_roi.m_min_scale < 0) m_roi.m_min_scale = 0;
    if(m_roi.m_max_scale > (int)scale_factors.size()) m_roi.m_max_scale = scale_factors.size();
    // if vector parts are empty, initialize them to full image pyramid
    if(!m_roi.vmin_x.size() && !m_roi.vmax_x.size() && !m_roi.vmin_y.size() && !m_roi.vmax_y.size()){
      InitROIvectors();
    }
    // if they are not empty but different from correct size, reinitialize them
    else {
      if((m_roi.vmin_x.size() != scale_factors.size()) || 
				(m_roi.vmax_x.size() != scale_factors.size()) || 
				(m_roi.vmin_y.size() != scale_factors.size()) || 
 				(m_roi.vmax_y.size() != scale_factors.size()) ){
				std::cerr << "MPIImagePyramid::SetROI():  Received inconsistent vector part of ROI! Resetting ROI vectors" << endl;
				InitROIvectors();
      }
      // Ok, suppose it has a valid vector part, then
      // make sure no vector parts go outside of image boundary
      else{
				for(unsigned int i=0; i < scale_factors.size(); ++i){
					if(m_roi.vmin_x[i] < 0) m_roi.vmin_x[i] = 0;
					if(m_roi.vmax_x[i] > m_image[0]->width) m_roi.vmax_x[i] = m_image[0]->width;
					if(m_roi.vmin_y[i] < 0) m_roi.vmin_y[i] = 0;
					if(m_roi.vmax_y[i] > m_image[0]->height) m_roi.vmax_y[i] = m_image[0]->height;
				}
      }
      // Make sure the ROI holds correct values for the absolute min and max
      for(unsigned int i=0; i < scale_factors.size(); ++i){
				if(m_roi.m_min_x > m_roi.vmin_x[i]) m_roi.m_min_x = m_roi.vmin_x[i];
				if(m_roi.m_max_x < m_roi.vmax_x[i]) m_roi.m_max_x = m_roi.vmax_x[i];
				if(m_roi.m_min_y > m_roi.vmin_y[i]) m_roi.m_min_y = m_roi.vmin_y[i];
				if(m_roi.m_max_y < m_roi.vmax_y[i]) m_roi.m_max_y = m_roi.vmax_y[i];
      }
    }
    return m_roi;
  }
  ROI getROI() const { return m_roi; }
  int getClosestScale(float input_scale_factor){
    // if scale_factors were a map, could use stl algorithms, but its small so whatever
    for(int i = 1; i<(int)scale_factors.size(); ++i)
      if(static_cast<float>(scale_factors[i])>input_scale_factor)
	return i-1;
    return scale_factors.size()-1;
  }
};


#endif




