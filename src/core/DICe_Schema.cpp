// @HEADER
// ************************************************************************
//
//               Digital Image Correlation Engine (DICe)
//                 Copyright 2015 Sandia Corporation.
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact: Dan Turner (dzturne@sandia.gov)
//
// ************************************************************************
// @HEADER

#include <DICe.h>
#include <DICe_Schema.h>
#include <DICe_ObjectiveZNSSD.h>
#include <DICe_PostProcessor.h>
#include <DICe_ParameterUtilities.h>
#include <DICe_FFT.h>

#include <Teuchos_ArrayRCP.hpp>

#include <ctime>
#include <iostream>
#include <fstream>
#include <algorithm>

#include <cassert>
#include <set>

#ifndef   DICE_DISABLE_BOOST_FILESYSTEM
#include    <boost/filesystem.hpp>
#endif

namespace DICe {

Schema::Schema(const std::string & refName,
  const std::string & defName,
  const Teuchos::RCP<Teuchos::ParameterList> & params)
{
  default_constructor_tasks(params);

  Teuchos::RCP<Teuchos::ParameterList> imgParams;
  if(params!=Teuchos::null) imgParams = params;
  else imgParams = Teuchos::rcp(new Teuchos::ParameterList());

  // construct the images
  // (the compute_image_gradients param is used by the image constructor)
  imgParams->set(DICe::compute_image_gradients,compute_ref_gradients_);
  imgParams->set(DICe::gauss_filter_images,gauss_filter_images_);
  ref_img_ = Teuchos::rcp( new Image(refName.c_str(),imgParams));
  prev_img_ = Teuchos::rcp( new Image(refName.c_str(),imgParams));
  // (the compute_image_gradients param is used by the image constructor)
  imgParams->set(DICe::compute_image_gradients,compute_def_gradients_);
  def_img_ = Teuchos::rcp( new Image(defName.c_str(),imgParams));
  if(ref_image_rotation_!=ZERO_DEGREES){
    ref_img_ = ref_img_->apply_rotation(ref_image_rotation_,imgParams);
    prev_img_ = prev_img_->apply_rotation(ref_image_rotation_,imgParams);
  }
  if(def_image_rotation_!=ZERO_DEGREES){
    def_img_ = def_img_->apply_rotation(def_image_rotation_,imgParams);
  }
  const int_t width = ref_img_->width();
  const int_t height = ref_img_->height();
  // require that the images are the same size
  TEUCHOS_TEST_FOR_EXCEPTION(width!=def_img_->width(),std::runtime_error,"  DICe ERROR: Images must be the same width.");
  TEUCHOS_TEST_FOR_EXCEPTION(height!=def_img_->height(),std::runtime_error,"  DICe ERROR: Images must be the same height.");
}

Schema::Schema(const int_t img_width,
  const int_t img_height,
  const Teuchos::ArrayRCP<intensity_t> refRCP,
  const Teuchos::ArrayRCP<intensity_t> defRCP,
  const Teuchos::RCP<Teuchos::ParameterList> & params)
{
  default_constructor_tasks(params);

  Teuchos::RCP<Teuchos::ParameterList> imgParams;
  if(params!=Teuchos::null) imgParams = params;
  else imgParams = Teuchos::rcp(new Teuchos::ParameterList());

  // (the compute_image_gradients param is used by the image constructor)
  imgParams->set(DICe::compute_image_gradients,compute_ref_gradients_);
  imgParams->set(DICe::gauss_filter_images,gauss_filter_images_);
  ref_img_ = Teuchos::rcp( new Image(img_width,img_height,refRCP,imgParams));
  prev_img_ = Teuchos::rcp( new Image(img_width,img_height,refRCP,imgParams));
  imgParams->set(DICe::compute_image_gradients,compute_def_gradients_);
  def_img_ = Teuchos::rcp( new Image(img_width,img_height,defRCP,imgParams));
  if(ref_image_rotation_!=ZERO_DEGREES){
    ref_img_ = ref_img_->apply_rotation(ref_image_rotation_,imgParams);
    prev_img_ = prev_img_->apply_rotation(ref_image_rotation_,imgParams);
  }
  if(def_image_rotation_!=ZERO_DEGREES){
    def_img_ = def_img_->apply_rotation(def_image_rotation_,imgParams);
  }
  // require that the images are the same size
  assert(!(ref_img_->width()<=0||ref_img_->width()!=def_img_->width()) && "  DICe ERROR: Images must be the same width and nonzero.");
  assert(!(ref_img_->height()<=0||ref_img_->height()!=def_img_->height()) && "  DICe ERROR: Images must be the same height and nonzero.");
}

Schema::Schema(Teuchos::RCP<Image> ref_img,
  Teuchos::RCP<Image> def_img,
  const Teuchos::RCP<Teuchos::ParameterList> & params)
{
  default_constructor_tasks(params);
  if(gauss_filter_images_){
    ref_img->gauss_filter();
    def_img->gauss_filter();
  }
  ref_img_ = ref_img;
  def_img_ = def_img;
  prev_img_ = ref_img;
  if(ref_image_rotation_!=ZERO_DEGREES){
    ref_img_ = ref_img_->apply_rotation(ref_image_rotation_);
    prev_img_ = prev_img_->apply_rotation(ref_image_rotation_);
  }
  if(def_image_rotation_!=ZERO_DEGREES){
    def_img_ = def_img_->apply_rotation(def_image_rotation_);
  }
  if(compute_ref_gradients_&&!ref_img_->has_gradients()){
    ref_img_->compute_gradients();
  }
  if(compute_def_gradients_&&!def_img_->has_gradients()){
    def_img_->compute_gradients();
  }
}

void
Schema::set_def_image(const std::string & defName){
  DEBUG_MSG("Schema: Resetting the deformed image");
  Teuchos::RCP<Teuchos::ParameterList> imgParams = Teuchos::rcp(new Teuchos::ParameterList());
  imgParams->set(DICe::gauss_filter_images,gauss_filter_images_);
  def_img_ = Teuchos::rcp( new Image(defName.c_str(),imgParams));
  if(def_image_rotation_!=ZERO_DEGREES){
    def_img_ = def_img_->apply_rotation(def_image_rotation_);
  }
}

void
Schema::set_def_image(Teuchos::RCP<Image> img){
  DEBUG_MSG("Schema: Resetting the deformed image");
  def_img_ = img;
  if(def_image_rotation_!=ZERO_DEGREES){
    def_img_ = def_img_->apply_rotation(def_image_rotation_);
  }
}

void
Schema::set_def_image(const int_t img_width,
  const int_t img_height,
  const Teuchos::ArrayRCP<intensity_t> defRCP){
  DEBUG_MSG("Schema:  Resetting the deformed image");
  assert(img_width>0);
  assert(img_height>0);
  def_img_ = Teuchos::rcp( new Image(img_width,img_height,defRCP));
  if(def_image_rotation_!=ZERO_DEGREES){
    def_img_ = def_img_->apply_rotation(def_image_rotation_);
  }
}

void
Schema::set_ref_image(const std::string & refName){
  DEBUG_MSG("Schema:  Resetting the reference image");
  Teuchos::RCP<Teuchos::ParameterList> imgParams = Teuchos::rcp(new Teuchos::ParameterList());
  imgParams->set(DICe::compute_image_gradients,true); // automatically compute the gradients if the ref image is changed
  ref_img_ = Teuchos::rcp( new Image(refName.c_str(),imgParams));
  if(ref_image_rotation_!=ZERO_DEGREES){
    ref_img_ = ref_img_->apply_rotation(ref_image_rotation_,imgParams);
  }
}

void
Schema::set_ref_image(const int_t img_width,
  const int_t img_height,
  const Teuchos::ArrayRCP<intensity_t> refRCP){
  DEBUG_MSG("Schema:  Resetting the reference image");
  assert(img_width>0);
  assert(img_height>0);
  Teuchos::RCP<Teuchos::ParameterList> imgParams = Teuchos::rcp(new Teuchos::ParameterList());
  imgParams->set(DICe::compute_image_gradients,true); // automatically compute the gradients if the ref image is changed
  ref_img_ = Teuchos::rcp( new Image(img_width,img_height,refRCP,imgParams));
  if(ref_image_rotation_!=ZERO_DEGREES){
    ref_img_ = ref_img_->apply_rotation(ref_image_rotation_,imgParams);
  }
}

void
Schema::default_constructor_tasks(const Teuchos::RCP<Teuchos::ParameterList> & params){
  data_num_points_ = 0;
  subset_dim_ = -1;
  step_size_x_ = -1;
  step_size_y_ = -1;
  mesh_size_ = -1.0;
  image_frame_ = 0;
  num_image_frames_ = -1;
  has_output_spec_ = false;
  is_initialized_ = false;
  analysis_type_ = LOCAL_DIC;
  target_field_descriptor_ = ALL_OWNED;
  distributed_fields_being_modified_ = false;
  has_post_processor_ = false;
  update_obstructed_pixels_each_iteration_ = false;
  normalize_gamma_with_active_pixels_ = false;
  gauss_filter_images_ = false;
  init_params_ = params;
  phase_cor_u_x_ = 0.0;
  phase_cor_u_y_ = 0.0;
  comm_ = Teuchos::rcp(new MultiField_Comm());
  path_file_names_ = Teuchos::rcp(new std::map<int_t,std::string>());
  skip_solve_flags_ = Teuchos::rcp(new std::map<int_t,bool>());
  motion_window_params_ = Teuchos::rcp(new std::map<int_t,Motion_Window_Params>());
  initial_gamma_threshold_ = -1.0;
  final_gamma_threshold_ = -1.0;
  path_distance_threshold_ = -1.0;
  set_params(params);
}

void
Schema::set_params(const Teuchos::RCP<Teuchos::ParameterList> & params){

  const int_t proc_rank = comm_->get_rank();

  if(params!=Teuchos::null){
    if(params->get<bool>(DICe::use_global_dic,false))
      analysis_type_=GLOBAL_DIC;
    // TODO make sure only one of these is active
  }

  // start with the default params and add any that are specified by the input params
  Teuchos::RCP<Teuchos::ParameterList> diceParams = Teuchos::rcp( new Teuchos::ParameterList("Schema_Correlation_Parameters") );

  if(analysis_type_==GLOBAL_DIC){
    TEUCHOS_TEST_FOR_EXCEPTION(true,std::invalid_argument,"Global DIC is not enabled");
  }
  else if(analysis_type_==LOCAL_DIC){
    bool use_tracking_defaults = false;
    if(params!=Teuchos::null){
      use_tracking_defaults = params->get<bool>(DICe::use_tracking_default_params,false);
    }
    // First set all of the params to their defaults in case the user does not specify them:
    if(use_tracking_defaults){
      tracking_default_params(diceParams.getRawPtr());
      if(proc_rank == 0) DEBUG_MSG("Initializing schema params with SL default parameters");
    }
    else{
      dice_default_params(diceParams.getRawPtr());
      if(proc_rank == 0) DEBUG_MSG("Initializing schema params with DICe default parameters");
    }
    // Overwrite any params that are specified by the params argument
    if(params!=Teuchos::null){
      // check that all the parameters are valid:
      // this should catch the case that the user misspelled one of the parameters:
      bool allParamsValid = true;
      for(Teuchos::ParameterList::ConstIterator it=params->begin();it!=params->end();++it){
        bool paramValid = false;
        for(int_t j=0;j<DICe::num_valid_correlation_params;++j){
          if(it->first==valid_correlation_params[j].name_){
            diceParams->setEntry(it->first,it->second); // overwrite the default value with argument param specified values
            paramValid = true;
          }
        }
        // catch post processor entries
        for(int_t j=0;j<DICe::num_valid_post_processor_params;++j){
          if(it->first==valid_post_processor_params[j]){
            diceParams->setEntry(it->first,it->second); // overwrite the default value with argument param specified values
            paramValid = true;
          }
        }
        if(!paramValid){
          allParamsValid = false;
          if(proc_rank == 0) std::cout << "Error: Invalid parameter: " << it->first << std::endl;
        }
      }
      if(!allParamsValid){
        if(proc_rank == 0) std::cout << "NOTE: valid parameters include: " << std::endl;
        for(int_t j=0;j<DICe::num_valid_correlation_params;++j){
          if(proc_rank == 0) std::cout << valid_correlation_params[j].name_ << std::endl;
        }
        for(int_t j=0;j<DICe::num_valid_post_processor_params;++j){
          if(proc_rank == 0) std::cout << valid_post_processor_params[j] << std::endl;
        }
      }
      TEUCHOS_TEST_FOR_EXCEPTION(!allParamsValid,std::invalid_argument,"Invalid parameter");
    }
  }
  else{
    assert(false && "Error, unrecognized analysis_type");
  }
#ifdef DICE_DEBUG_MSG
  if(proc_rank == 0) {
    std::cout << "Full set of correlation parameters: " << std::endl;
    diceParams->print(std::cout);
  }
#endif

  gauss_filter_images_ = diceParams->get<bool>(DICe::gauss_filter_images,false);
  compute_ref_gradients_ = diceParams->get<bool>(DICe::compute_ref_gradients,false);
  compute_def_gradients_ = diceParams->get<bool>(DICe::compute_def_gradients,false);
  if(diceParams->get<bool>(DICe::compute_image_gradients,false)) { // this flag turns them both on
    compute_ref_gradients_ = true;
    compute_def_gradients_ = true;
  }
  assert(diceParams->isParameter(DICe::projection_method));
  projection_method_ = diceParams->get<Projection_Method>(DICe::projection_method);
  assert(diceParams->isParameter(DICe::interpolation_method));
  interpolation_method_ = diceParams->get<Interpolation_Method>(DICe::interpolation_method);
  assert(diceParams->isParameter(DICe::max_evolution_iterations));
  max_evolution_iterations_ = diceParams->get<int_t>(DICe::max_evolution_iterations);
  assert(diceParams->isParameter(DICe::max_solver_iterations_fast));
  max_solver_iterations_fast_ = diceParams->get<int_t>(DICe::max_solver_iterations_fast);
  assert(diceParams->isParameter(DICe::fast_solver_tolerance));
  fast_solver_tolerance_ = diceParams->get<double>(DICe::fast_solver_tolerance);
  // make sure image gradients are on at least for the reference image for any gradient based optimization routine
  assert(diceParams->isParameter(DICe::optimization_method));
  optimization_method_ = diceParams->get<Optimization_Method>(DICe::optimization_method);
  assert(diceParams->isParameter(DICe::correlation_routine));
  correlation_routine_ = diceParams->get<Correlation_Routine>(DICe::correlation_routine);
  assert(diceParams->isParameter(DICe::initialization_method));
  initialization_method_ = diceParams->get<Initialization_Method>(DICe::initialization_method);
  assert(diceParams->isParameter(DICe::max_solver_iterations_robust));
  max_solver_iterations_robust_ = diceParams->get<int_t>(DICe::max_solver_iterations_robust);
  assert(diceParams->isParameter(DICe::robust_solver_tolerance));
  robust_solver_tolerance_ = diceParams->get<double>(DICe::robust_solver_tolerance);
  assert(diceParams->isParameter(DICe::skip_solve_gamma_threshold));
  skip_solve_gamma_threshold_ = diceParams->get<double>(DICe::skip_solve_gamma_threshold);
  assert(diceParams->isParameter(DICe::initial_gamma_threshold));
  initial_gamma_threshold_ = diceParams->get<double>(DICe::initial_gamma_threshold);
  assert(diceParams->isParameter(DICe::final_gamma_threshold));
  final_gamma_threshold_ = diceParams->get<double>(DICe::final_gamma_threshold);
  assert(diceParams->isParameter(DICe::path_distance_threshold));
  path_distance_threshold_ = diceParams->get<double>(DICe::path_distance_threshold);
  assert(diceParams->isParameter(DICe::disp_jump_tol));
  disp_jump_tol_ = diceParams->get<double>(DICe::disp_jump_tol);
  assert(diceParams->isParameter(DICe::theta_jump_tol));
  theta_jump_tol_ = diceParams->get<double>(DICe::theta_jump_tol);
  assert(diceParams->isParameter(DICe::robust_delta_disp));
  robust_delta_disp_ = diceParams->get<double>(DICe::robust_delta_disp);
  assert(diceParams->isParameter(DICe::robust_delta_theta));
  robust_delta_theta_ = diceParams->get<double>(DICe::robust_delta_theta);
  assert(diceParams->isParameter(DICe::enable_translation));
  enable_translation_ = diceParams->get<bool>(DICe::enable_translation);
  assert(diceParams->isParameter(DICe::enable_rotation));
  enable_rotation_ = diceParams->get<bool>(DICe::enable_rotation);
  assert(diceParams->isParameter(DICe::enable_normal_strain));
  enable_normal_strain_ = diceParams->get<bool>(DICe::enable_normal_strain);
  assert(diceParams->isParameter(DICe::enable_shear_strain));
  enable_shear_strain_ = diceParams->get<bool>(DICe::enable_shear_strain);
  assert(diceParams->isParameter(DICe::output_deformed_subset_images));
  output_deformed_subset_images_ = diceParams->get<bool>(DICe::output_deformed_subset_images);
  assert(diceParams->isParameter(DICe::output_deformed_subset_intensity_images));
  output_deformed_subset_intensity_images_ = diceParams->get<bool>(DICe::output_deformed_subset_intensity_images);
  assert(diceParams->isParameter(DICe::output_evolved_subset_images));
  output_evolved_subset_images_ = diceParams->get<bool>(DICe::output_evolved_subset_images);
  assert(diceParams->isParameter(DICe::use_subset_evolution));
  use_subset_evolution_ = diceParams->get<bool>(DICe::use_subset_evolution);
  assert(diceParams->isParameter(DICe::obstruction_buffer_size));
  obstruction_buffer_size_ = diceParams->get<int_t>(DICe::obstruction_buffer_size);
  assert(diceParams->isParameter(DICe::pixel_integration_order));
  pixel_integration_order_ = diceParams->get<int_t>(DICe::pixel_integration_order);
  assert(diceParams->isParameter(DICe::obstruction_skin_factor));
  obstruction_skin_factor_ = diceParams->get<double>(DICe::obstruction_skin_factor);
  assert(diceParams->isParameter(DICe::use_objective_regularization));
  use_objective_regularization_ = diceParams->get<bool>(DICe::use_objective_regularization);
  assert(diceParams->isParameter(DICe::objective_regularization_factor));
  objective_regularization_factor_ = diceParams->get<double>(DICe::objective_regularization_factor);
  assert(diceParams->isParameter(DICe::update_obstructed_pixels_each_iteration));
  update_obstructed_pixels_each_iteration_ = diceParams->get<bool>(DICe::update_obstructed_pixels_each_iteration);
  if(update_obstructed_pixels_each_iteration_)
    DEBUG_MSG("Obstructed pixel information will be updated each iteration.");
  assert(diceParams->isParameter(DICe::normalize_gamma_with_active_pixels));
  normalize_gamma_with_active_pixels_ = diceParams->get<bool>(DICe::normalize_gamma_with_active_pixels);
  assert(diceParams->isParameter(DICe::rotate_ref_image_90));
  assert(diceParams->isParameter(DICe::rotate_ref_image_180));
  assert(diceParams->isParameter(DICe::rotate_ref_image_270));
  assert(diceParams->isParameter(DICe::rotate_def_image_90));
  assert(diceParams->isParameter(DICe::rotate_def_image_180));
  assert(diceParams->isParameter(DICe::rotate_def_image_270));
  // last one read wins here:
  ref_image_rotation_ = ZERO_DEGREES;
  def_image_rotation_ = ZERO_DEGREES;
  if(diceParams->get<bool>(DICe::rotate_ref_image_90)) ref_image_rotation_ = NINTY_DEGREES;
  if(diceParams->get<bool>(DICe::rotate_ref_image_180)) ref_image_rotation_ = ONE_HUNDRED_EIGHTY_DEGREES;
  if(diceParams->get<bool>(DICe::rotate_ref_image_270)) ref_image_rotation_ = TWO_HUNDRED_SEVENTY_DEGREES;
  if(diceParams->get<bool>(DICe::rotate_def_image_90)) def_image_rotation_ = NINTY_DEGREES;
  if(diceParams->get<bool>(DICe::rotate_def_image_180)) def_image_rotation_ = ONE_HUNDRED_EIGHTY_DEGREES;
  if(diceParams->get<bool>(DICe::rotate_def_image_270)) def_image_rotation_ = TWO_HUNDRED_SEVENTY_DEGREES;
  if(normalize_gamma_with_active_pixels_)
    DEBUG_MSG("Gamma values will be normalized by the number of active pixels.");
  if(analysis_type_==GLOBAL_DIC){
    compute_ref_gradients_ = true;
    assert(diceParams->isParameter(DICe::use_hvm_stabilization));
    use_hvm_stabilization_ = diceParams->get<bool>(DICe::use_hvm_stabilization);
  }
  else{
    if(optimization_method_!=DICe::SIMPLEX) {
      compute_ref_gradients_ = true;
    }
  }
  // create all the necessary post processors
  // create any of the post processors that may have been requested
  if(diceParams->isParameter(DICe::post_process_vsg_strain)){
    Teuchos::ParameterList sublist = diceParams->sublist(DICe::post_process_vsg_strain);
    Teuchos::RCP<Teuchos::ParameterList> ppParams = Teuchos::rcp( new Teuchos::ParameterList());
    for(Teuchos::ParameterList::ConstIterator it=sublist.begin();it!=sublist.end();++it){
      ppParams->setEntry(it->first,it->second);
    }
    Teuchos::RCP<VSG_Strain_Post_Processor> vsg_ptr = Teuchos::rcp (new VSG_Strain_Post_Processor(this,ppParams));
    post_processors_.push_back(vsg_ptr);
  }
  if(diceParams->isParameter(DICe::post_process_nlvc_strain)){
    Teuchos::ParameterList sublist = diceParams->sublist(DICe::post_process_nlvc_strain);
    Teuchos::RCP<Teuchos::ParameterList> ppParams = Teuchos::rcp( new Teuchos::ParameterList());
    for(Teuchos::ParameterList::ConstIterator it=sublist.begin();it!=sublist.end();++it){
      ppParams->setEntry(it->first,it->second);
    }
    Teuchos::RCP<NLVC_Strain_Post_Processor> nlvc_ptr = Teuchos::rcp (new NLVC_Strain_Post_Processor(this,ppParams));
    post_processors_.push_back(nlvc_ptr);
  }
  if(diceParams->isParameter(DICe::post_process_keys4_strain)){
    Teuchos::ParameterList sublist = diceParams->sublist(DICe::post_process_keys4_strain);
    Teuchos::RCP<Teuchos::ParameterList> ppParams = Teuchos::rcp( new Teuchos::ParameterList());
    for(Teuchos::ParameterList::ConstIterator it=sublist.begin();it!=sublist.end();++it){
      ppParams->setEntry(it->first,it->second);
    }
    Teuchos::RCP<Keys4_Strain_Post_Processor> keys4_ptr = Teuchos::rcp (new Keys4_Strain_Post_Processor(this,ppParams));
    post_processors_.push_back(keys4_ptr);
  }
  if(diceParams->isParameter(DICe::post_process_global_strain)){
    Teuchos::ParameterList sublist = diceParams->sublist(DICe::post_process_global_strain);
    Teuchos::RCP<Teuchos::ParameterList> ppParams = Teuchos::rcp( new Teuchos::ParameterList());
    for(Teuchos::ParameterList::ConstIterator it=sublist.begin();it!=sublist.end();++it){
      ppParams->setEntry(it->first,it->second);
    }
    Teuchos::RCP<Global_Strain_Post_Processor> global_ptr = Teuchos::rcp (new Global_Strain_Post_Processor(this,ppParams));
    post_processors_.push_back(global_ptr);
  }
  if(post_processors_.size()>0) has_post_processor_ = true;

  Teuchos::RCP<Teuchos::ParameterList> outputParams;
  if(diceParams->isParameter(DICe::output_spec)){
    if(proc_rank == 0) DEBUG_MSG("Output spec was provided by user");
    // Strip output params sublist out of params
    Teuchos::ParameterList output_sublist = diceParams->sublist(DICe::output_spec);
    outputParams = Teuchos::rcp( new Teuchos::ParameterList());
    // iterate the sublis and add the params to the output params:
    for(Teuchos::ParameterList::ConstIterator it=output_sublist.begin();it!=output_sublist.end();++it){
      outputParams->setEntry(it->first,it->second);
    }
  }
  // create the output spec:
  const std::string delimiter = diceParams->get<std::string>(DICe::output_delimiter," ");
  const bool omit_row_id = diceParams->get<bool>(DICe::omit_output_row_id,false);
  output_spec_ = Teuchos::rcp(new DICe::Output_Spec(this,omit_row_id,outputParams,delimiter));
  has_output_spec_ = true;
}

void
Schema::initialize(const int_t step_size_x,
  const int_t step_size_y,
  const int_t subset_size){

  assert(!is_initialized_ && "Error: this schema is already initialized.");
  assert(subset_size>0 && "  Error: width cannot be equal to or less than zero.");
  step_size_x_ = step_size_x;
  step_size_y_ = step_size_y;

  const int_t img_width = ref_img_->width();
  const int_t img_height = ref_img_->height();
  // create a buffer the size of one view along all edges
  const int_t trimmedWidth = img_width - 2*subset_size;
  const int_t trimmedHeight = img_height - 2*subset_size;
  // set up the control points
  assert(step_size_x > 0 && "  DICe ERROR: step size x is <= 0");
  assert(step_size_y > 0 && "  DICe ERROR: step size y is <= 0");
  const int_t numPointsX = trimmedWidth  / step_size_x + 1;
  const int_t numPointsY = trimmedHeight / step_size_y + 1;
  assert(numPointsX > 0 && "  DICe ERROR: numPointsX <= 0.");
  assert(numPointsY > 0 && "  DICe ERROR: numPointsY <= 0.");

  const int_t num_pts = numPointsX * numPointsY;

  initialize(num_pts,subset_size);
  assert(data_num_points_==num_pts);

  int_t x_it=0, y_it=0, x_coord=0, y_coord=0;
  for (int_t i=0;i<num_pts;++i)
  {
     y_it = i / numPointsX;
     x_it = i - (y_it*numPointsX);
     x_coord = subset_dim_ + x_it * step_size_x_ -1;
     y_coord = subset_dim_ + y_it * step_size_y_ -1;
     field_value(i,COORDINATE_X) = x_coord;
     field_value(i,COORDINATE_Y) = y_coord;
  }
}

void
Schema::initialize(const int_t num_pts,
  const int_t subset_size,
  Teuchos::RCP<std::map<int_t,Conformal_Area_Def> > conformal_subset_defs,
  Teuchos::RCP<std::vector<int_t> > neighbor_ids){
  assert(def_img_->width()==ref_img_->width());
  assert(def_img_->height()==ref_img_->height());
  if(is_initialized_){
    assert(data_num_points_>0);
    assert(fields_->get_num_fields()==MAX_FIELD_NAME);
    assert(fields_nm1_->get_num_fields()==MAX_FIELD_NAME);
    return;  // no need to initialize if already done
  }
  // TODO find some way to address this (for constrained optimization, the schema doesn't need any fields)
  //assert(num_pts>0);
  data_num_points_ = num_pts;
  subset_dim_ = subset_size;

  // evenly distributed one-to-one map
  dist_map_ = Teuchos::rcp(new MultiField_Map(data_num_points_,0,*comm_));

  // all owned map (not one-to-one)
  Teuchos::Array<int_t> all_subsets(data_num_points_);
  for(int_t i=0;i<data_num_points_;++i)
    all_subsets[i] = i;
  all_map_ = Teuchos::rcp(new MultiField_Map(-1,all_subsets.view(0,all_subsets.size()),0,*comm_));

  // if there are blocking subsets, they need to be on the same processor and put in order:
  create_obstruction_dist_map();

  create_seed_dist_map(neighbor_ids);

  importer_ = Teuchos::rcp(new MultiField_Importer(*dist_map_,*all_map_));
  exporter_ = Teuchos::rcp(new MultiField_Exporter(*all_map_,*dist_map_));
  seed_importer_ = Teuchos::rcp(new MultiField_Importer(*seed_dist_map_,*all_map_));
  seed_exporter_ = Teuchos::rcp(new MultiField_Exporter(*all_map_,*seed_dist_map_));
  fields_ = Teuchos::rcp(new MultiField(all_map_,MAX_FIELD_NAME,true));
  fields_nm1_ = Teuchos::rcp(new MultiField(all_map_,MAX_FIELD_NAME,true));
#if DICE_MPI
  dist_fields_ = Teuchos::rcp(new MultiField(dist_map_,MAX_FIELD_NAME,true));
  dist_fields_nm1_ = Teuchos::rcp(new MultiField(dist_map_,MAX_FIELD_NAME,true));
  seed_dist_fields_ = Teuchos::rcp(new MultiField(seed_dist_map_,MAX_FIELD_NAME,true));
  seed_dist_fields_nm1_ = Teuchos::rcp(new MultiField(seed_dist_map_,MAX_FIELD_NAME,true));
#endif
  // initialize the conformal subset map to avoid havng to check if its null always
  if(conformal_subset_defs==Teuchos::null)
    conformal_subset_defs_ = Teuchos::rcp(new std::map<int_t,DICe::Conformal_Area_Def>);
  else
    conformal_subset_defs_ = conformal_subset_defs;

  assert(data_num_points_ >= (int_t)conformal_subset_defs_->size() && "  DICe ERROR: data is not the right size, "
      "conformal_subset_defs_.size() is too large for the data array");
  // ensure that the ids in conformal subset defs are valid:
  typename std::map<int_t,Conformal_Area_Def>::iterator it = conformal_subset_defs_->begin();
  for( ;it!=conformal_subset_defs_->end();++it){
    assert(it->first >= 0);
    assert(it->first < data_num_points_);
  }
  // ensure that a subset size was specified if not all subsets are conformal:
  if(analysis_type_==LOCAL_DIC&&(int_t)conformal_subset_defs_->size()<data_num_points_){
    assert(subset_size > 0);
  }

  // initialize the post processors
  for(size_t i=0;i<post_processors_.size();++i)
    post_processors_[i]->initialize();

  // initialize the optimization initializers (one for each subset)
  opt_initializers_.resize(data_num_points_);
  for(size_t i=0;i<opt_initializers_.size();++i)
    opt_initializers_[i] = Teuchos::null;

  motion_detectors_.resize(data_num_points_);
  for(size_t i=0;i<motion_detectors_.size();++i)
    motion_detectors_[i] = Teuchos::null;

  is_initialized_ = true;

  if(neighbor_ids!=Teuchos::null)
    for(int_t i=0;i<data_num_points_;++i){
      field_value(i,DICe::NEIGHBOR_ID)  = (*neighbor_ids)[i];
    }
}

void
Schema::create_obstruction_dist_map(){
  if(obstructing_subset_ids_==Teuchos::null) return;

  const int_t proc_id = comm_->get_rank();
  const int_t num_procs = comm_->get_size();

  if(proc_id == 0) DEBUG_MSG("Subsets have obstruction dependencies.");
  // set up the groupings of subset ids that have to stay together:
  // Note: this assumes that the obstructions are only one relation deep
  // i.e. the blocking subset cannot itself have a subset that blocks it
  // TODO address this to make it more general
  std::set<int_t> eligible_ids;
  for(int_t i=0;i<data_num_points_;++i)
    eligible_ids.insert(i);
  std::vector<std::set<int_t> > obstruction_groups;
  std::map<int_t,int_t> earliest_id_can_appear;
  std::set<int_t> assigned_to_a_group;
  typename std::map<int_t,std::vector<int_t> >::iterator map_it = obstructing_subset_ids_->begin();
  for(;map_it!=obstructing_subset_ids_->end();++map_it){
    int_t greatest_subset_id_among_obst = 0;
    for(size_t j=0;j<map_it->second.size();++j)
      if(map_it->second[j] > greatest_subset_id_among_obst) greatest_subset_id_among_obst = map_it->second[j];
    earliest_id_can_appear.insert(std::pair<int_t,int_t>(map_it->first,greatest_subset_id_among_obst));

    if(assigned_to_a_group.find(map_it->first)!=assigned_to_a_group.end()) continue;
    std::set<int_t> dependencies;
    dependencies.insert(map_it->first);
    eligible_ids.erase(map_it->first);
    // gather for all the dependencies for this subset
    for(size_t j=0;j<map_it->second.size();++j){
      dependencies.insert(map_it->second[j]);
      eligible_ids.erase(map_it->second[j]);
    }
    // no search all the other obstruction sets for any ids currently in the dependency list
    typename std::set<int_t>::iterator dep_it = dependencies.begin();
    for(;dep_it!=dependencies.end();++dep_it){
      typename std::map<int_t,std::vector<int_t> >::iterator search_it = obstructing_subset_ids_->begin();
      for(;search_it!=obstructing_subset_ids_->end();++search_it){
        if(assigned_to_a_group.find(search_it->first)!=assigned_to_a_group.end()) continue;
        // if any of the ids are in the current dependency list, add the whole set:
        bool match_found = false;
        if(*dep_it==search_it->first) match_found = true;
        for(size_t k=0;k<search_it->second.size();++k){
          if(*dep_it==search_it->second[k]) match_found = true;
        }
        if(match_found){
          dependencies.insert(search_it->first);
          eligible_ids.erase(search_it->first);
          for(size_t k=0;k<search_it->second.size();++k){
            dependencies.insert(search_it->second[k]);
            eligible_ids.erase(search_it->second[k]);
          }
          // reset the dependency index because more items were added to the list
          dep_it = dependencies.begin();
          // remove this set of obstruction ids since they have already been added to a group
          assigned_to_a_group.insert(search_it->first);
        } // match found
      } // obstruction set
    } // dependency it
    obstruction_groups.push_back(dependencies);
  } // outer obstruction set it
  if(proc_id == 0) DEBUG_MSG("[PROC " << proc_id << "] There are " << obstruction_groups.size() << " obstruction groupings: ");
  std::stringstream ss;
  for(size_t i=0;i<obstruction_groups.size();++i){
    ss << "[PROC " << proc_id << "] Group: " << i << std::endl;
    typename std::set<int_t>::iterator j = obstruction_groups[i].begin();
    for(;j!=obstruction_groups[i].end();++j){
      ss << "[PROC " << proc_id << "]   id: " << *j << std::endl;
    }
  }
  ss << "[PROC " << proc_id << "] Eligible ids: " << std::endl;
  for(typename std::set<int_t>::iterator elig_it=eligible_ids.begin();elig_it!=eligible_ids.end();++elig_it){
    ss << "[PROC " << proc_id << "]   " << *elig_it << std::endl;
  }
  if(proc_id == 0) DEBUG_MSG(ss.str());

  // divy up the obstruction groups among the processors:
  int_t obst_group_gid = 0;
  std::vector<std::set<int_t> > local_subset_ids(num_procs);
  while(obst_group_gid < (int_t)obstruction_groups.size()){
    for(int_t p_id=0;p_id<num_procs;++p_id){
      if(obst_group_gid < (int_t)obstruction_groups.size()){
        //if(p_id==proc_id){
        local_subset_ids[p_id].insert(obstruction_groups[obst_group_gid].begin(),obstruction_groups[obst_group_gid].end());
        //}
        obst_group_gid++;
      }
      else break;
    }
  }
  // assign the rest based on who has the least amount of subsets
  for(typename std::set<int_t>::iterator elig_it = eligible_ids.begin();elig_it!=eligible_ids.end();++elig_it){
    int_t proc_with_fewest_subsets = 0;
    int_t lowest_num_subsets = data_num_points_;
    for(int_t i=0;i<num_procs;++i){
      if((int_t)local_subset_ids[i].size() <= lowest_num_subsets){
        lowest_num_subsets = local_subset_ids[i].size();
        proc_with_fewest_subsets = i;
      }
    }
    local_subset_ids[proc_with_fewest_subsets].insert(*elig_it);
  }
  // order the subset ids so that they respect the dependencies:
  std::vector<int_t> local_ids;
  typename std::set<int_t>::iterator set_it = local_subset_ids[proc_id].begin();
  for(;set_it!=local_subset_ids[proc_id].end();++set_it){
    // not in the list of subsets with blockers
    if(obstructing_subset_ids_->find(*set_it)==obstructing_subset_ids_->end()){
      local_ids.push_back(*set_it);
    }
    // in the list of subsets with blockers, but has no blocking ids
    else if(obstructing_subset_ids_->find(*set_it)->second.size()==0){
      local_ids.push_back(*set_it);
    }

  }
  set_it = local_subset_ids[proc_id].begin();
  for(;set_it!=local_subset_ids[proc_id].end();++set_it){
    if(obstructing_subset_ids_->find(*set_it)!=obstructing_subset_ids_->end()){
      if(obstructing_subset_ids_->find(*set_it)->second.size()>0){
        assert(earliest_id_can_appear.find(*set_it)!=earliest_id_can_appear.end());
        local_ids.push_back(*set_it);
      }
    }
  }

  ss.str(std::string());
  ss.clear();
  ss << "[PROC " << proc_id << "] Has the following subset ids: " << std::endl;
  for(size_t i=0;i<local_ids.size();++i){
    ss << "[PROC " << proc_id << "] " << local_ids[i] <<  std::endl;
  }
  DEBUG_MSG(ss.str());

  Teuchos::ArrayView<const int_t> lids_grouped_by_obstruction(&local_ids[0],local_ids.size());
  dist_map_ = Teuchos::rcp(new MultiField_Map(data_num_points_,lids_grouped_by_obstruction,0,*comm_));
  //dist_map_->describe();
  assert(dist_map_->is_one_to_one());

  // if this is a serial run, the ordering must be changed too
  if(num_procs==1)
    all_map_ = Teuchos::rcp(new MultiField_Map(data_num_points_,lids_grouped_by_obstruction,0,*comm_));
  //all_map_->describe();
}

void
Schema::create_seed_dist_map(Teuchos::RCP<std::vector<int_t> > neighbor_ids){
  // distribution according to seeds map (one-to-one, not all procs have entries)
  // If the initialization method is USE_NEIGHBOR_VALUES or USE_NEIGHBOR_VALUES_FIRST_STEP, the
  // first step has to have a special map that keeps all subsets that use a particular seed
  // on the same processor (the parallelism is limited to the number of seeds).
  const int_t proc_id = comm_->get_rank();
  const int_t num_procs = comm_->get_size();

  if(neighbor_ids!=Teuchos::null){
    // catch the case that this is an TRACKING_ROUTINE run, but seed values were specified for
    // the individual subsets. In that case, the seed map is not necessary because there are
    // no initializiation dependencies among subsets, but the seed map will still be used since it
    // will be activated when seeds are specified for a subset.
    if(obstructing_subset_ids_!=Teuchos::null){
      if(obstructing_subset_ids_->size()>0){
        bool print_warning = false;
        for(size_t i=0;i<neighbor_ids->size();++i){
          if((*neighbor_ids)[i]!=-1) print_warning = true;
        }
        if(print_warning && proc_id==0){
          std::cout << "*** Waring: Seed values were specified for an anlysis with obstructing subsets. " << std::endl;
          std::cout << "            These values will be used to initialize subsets for which a seed has been specified, but the seed map " << std::endl;
          std::cout << "            will be set to the distributed map because grouping subsets by obstruction trumps seed ordering." << std::endl;
          std::cout << "            Seed dependencies between neighbors will not be enforced." << std::endl;
        }
        seed_dist_map_ = dist_map_;
        return;
      }
    }
    std::vector<int_t> local_subset_gids_grouped_by_roi;
    assert((int_t)neighbor_ids->size()==data_num_points_);
    std::vector<int_t> this_group_gids;
    std::vector<std::vector<int_t> > seed_groupings;
    std::vector<std::vector<int_t> > local_seed_groupings;
    for(int_t i=data_num_points_-1;i>=0;--i){
      this_group_gids.push_back(i);
      // if this subset is a seed, break this grouping and insert it in the set
      if((*neighbor_ids)[i]==-1){
        seed_groupings.push_back(this_group_gids);
        this_group_gids.clear();
      }
    }
    // TODO order the sets by their sizes and load balance better:
    // divy up the seed_groupings round-robin style:
    int_t group_gid = 0;
    int_t local_total_id_list_size = 0;
    while(group_gid < (int_t)seed_groupings.size()){
      // reverse the order so the subsets are computed from the seed out
      for(int_t p_id=0;p_id<num_procs;++p_id){
        if(group_gid < (int_t)seed_groupings.size()){
          if(p_id==proc_id){
            std::reverse(seed_groupings[group_gid].begin(), seed_groupings[group_gid].end());
            local_seed_groupings.push_back(seed_groupings[group_gid]);
            local_total_id_list_size += seed_groupings[group_gid].size();
          }
          group_gid++;
        }
        else break;
      }
    }
    DEBUG_MSG("[PROC " << proc_id << "] Has " << local_seed_groupings.size() << " local seed grouping(s)");
    for(size_t i=0;i<local_seed_groupings.size();++i){
      DEBUG_MSG("[PROC " << proc_id << "] local group id: " << i);
      for(size_t j=0;j<local_seed_groupings[i].size();++j){
        DEBUG_MSG("[PROC " << proc_id << "] gid: " << local_seed_groupings[i][j] );
      }
    }
    // concat local subset ids:
    local_subset_gids_grouped_by_roi.reserve(local_total_id_list_size);
    for(size_t i=0;i<local_seed_groupings.size();++i){
      local_subset_gids_grouped_by_roi.insert( local_subset_gids_grouped_by_roi.end(),
        local_seed_groupings[i].begin(),
        local_seed_groupings[i].end());
    }
    Teuchos::ArrayView<const int_t> lids_grouped_by_roi(&local_subset_gids_grouped_by_roi[0],local_total_id_list_size);
    seed_dist_map_ = Teuchos::rcp(new MultiField_Map(data_num_points_,lids_grouped_by_roi,0,*comm_));
  } // end has_neighbor_ids
  else{
    seed_dist_map_ = dist_map_;
  }
}

void
Schema::execute_correlation(){

  // make sure the data is ready to go since it may have been initialized externally by an api
  assert(is_initialized_);
  assert(fields_->get_num_fields()==MAX_FIELD_NAME);
  assert(fields_nm1_->get_num_fields()==MAX_FIELD_NAME);
  assert(data_num_points_>0);

  const int_t proc_id = comm_->get_rank();
  const int_t num_procs = comm_->get_size();

  DEBUG_MSG("********************");
  std::stringstream progress;
  progress << "[PROC " << proc_id << " of " << num_procs << "] IMAGE FRAME " << image_frame_;
  if(num_image_frames_>0)
    progress << " of " << num_image_frames_;
  DEBUG_MSG(progress.str());
  DEBUG_MSG("********************");

  int_t num_local_subsets = this_proc_subset_global_ids_.size();

  // reset the motion detectors for each subset if used
  for(size_t i=0;i<motion_detectors_.size();++i)
    if(motion_detectors_[i]!=Teuchos::null){
      DEBUG_MSG("Resetting motion detector: " << i);
      motion_detectors_[i]->reset();
    }

  // PARALLEL CASE:
  if(num_procs >1){
    // first pass for a USE_FILED_VALUES run sets up the local subset list
    // for all subsequent frames, the list remains unchanged. For this case, it
    // doesn't matter if seeding is used, because neighbor values are not needed
    if(initialization_method_==USE_FIELD_VALUES){
      target_field_descriptor_ = DISTRIBUTED;
      if(this_proc_subset_global_ids_.size()==0){
        num_local_subsets = dist_map_->get_num_local_elements();
        this_proc_subset_global_ids_ = dist_map_->get_local_element_list();
      }
    }
    // if seeding is used and the init method is USE_NEIGHBOR_VALUES_FIRST_STEP_ONLY, the first
    // frame has to be serial, the rest can be parallel
    // TODO if multiple ROIs are used, the ROIs can be split across processors
    else if(initialization_method_==USE_NEIGHBOR_VALUES_FIRST_STEP_ONLY){
      if(image_frame_==0){
        target_field_descriptor_ = DISTRIBUTED_GROUPED_BY_SEED;
        num_local_subsets = seed_dist_map_->get_num_local_elements();
        this_proc_subset_global_ids_ = seed_dist_map_->get_local_element_list();
      }
      else if(image_frame_==1){
        target_field_descriptor_ = DISTRIBUTED;
        num_local_subsets = dist_map_->get_num_local_elements();
        this_proc_subset_global_ids_ = dist_map_->get_local_element_list();
      }
      // otherwise nothing needs to be done since the maps will not need to change after step 1
    }
    /// For use neighbor values, the run has to be serial for each grouping that has a seed
    else if(initialization_method_==USE_NEIGHBOR_VALUES){
      if(image_frame_==0){
        target_field_descriptor_ = DISTRIBUTED_GROUPED_BY_SEED;
        num_local_subsets = seed_dist_map_->get_num_local_elements();
        this_proc_subset_global_ids_ = seed_dist_map_->get_local_element_list();
      }
    }
    else{
      assert(false && "Error: unknown initialization_method in execute correlation");
    }
  }

  // SERIAL CASE:

  else{
    if(image_frame_==0){
      target_field_descriptor_ = ALL_OWNED;
      num_local_subsets = all_map_->get_num_local_elements();
      this_proc_subset_global_ids_ = all_map_->get_local_element_list();
    }
  }
#ifdef DICE_DEBUG_MSG
  std::stringstream message;
  message << std::endl;
  for(int_t i=0;i<num_local_subsets;++i){
    message << "[PROC " << proc_id << "] Owns subset global id: " << this_proc_subset_global_ids_[i] << std::endl;
  }
  DEBUG_MSG(message.str());
#endif
  DEBUG_MSG("[PROC " << proc_id << "] has target_field_descriptor " << target_field_descriptor_);

  // Complete the set up activities for the post processors
  if(image_frame_==0){
    for(size_t i=0;i<post_processors_.size();++i){
      post_processors_[i]->pre_execution_tasks();
    }
  }

  // sync the fields:
  sync_fields_all_to_dist();

  // if requested, do a phase correlation of the images to get the initial guess for u_x and u_y:
  if(initialization_method_==USE_PHASE_CORRELATION){
    DICe::phase_correlate_x_y(prev_img_,def_img_,phase_cor_u_x_,phase_cor_u_y_);
    DEBUG_MSG(" - phase correlation initial displacements ux: " << phase_cor_u_x_ << " uy: " << phase_cor_u_y_);
  }

  // The generic routine is typically used when the dataset involves numerous subsets,
  // but only a small number of images. In this case it's more efficient to re-allocate the
  // objectives at every step, since making them static would consume a lot of memory
  if(correlation_routine_==GENERIC_ROUTINE){
    for(int_t subset_index=0;subset_index<num_local_subsets;++subset_index){
      Teuchos::RCP<Objective> obj = Teuchos::rcp(new Objective_ZNSSD(this,
        this_proc_subset_global_ids_[subset_index]));
      generic_correlation_routine(obj);
    }
  }
  // In this routine there are usually only a handful of subsets, but thousands of images.
  // In this case it is a lot more efficient to make the objectives static since there won't
  // be very many of them, and we can avoid the allocation cost at every step
  else if(correlation_routine_==TRACKING_ROUTINE){
    // construct the static objectives if they haven't already been constructed
    if(obj_vec_.empty()){
      for(int_t subset_index=0;subset_index<num_local_subsets;++subset_index){
        DEBUG_MSG("[PROC " << proc_id << "] Adding objective to obj_vec_ " << this_proc_subset_global_ids_[subset_index]);
        obj_vec_.push_back(Teuchos::rcp(new Objective_ZNSSD(this,
          this_proc_subset_global_ids_[subset_index])));
      }
    }
    assert((int_t)obj_vec_.size()==num_local_subsets);
    // now run the correlations:
    for(int_t subset_index=0;subset_index<num_local_subsets;++subset_index){
      check_for_blocking_subsets(this_proc_subset_global_ids_[subset_index]);
      generic_correlation_routine(obj_vec_[subset_index]);
    }
    if(output_deformed_subset_images_)
      write_deformed_subsets_image();
    prev_img_=def_img_;
  }
  else
    assert(false && "  DICe ERROR: unknown correlation routine.");

  // sync the fields
  sync_fields_dist_to_all();

  if(proc_id==0){
    for(int_t subset_index=0;subset_index<data_num_points_;++subset_index){
      DEBUG_MSG("[PROC " << proc_id << "] Subset " << subset_index << " synced-up solution after execute_correlation() done, u: " <<
        field_value(subset_index,DISPLACEMENT_X) << " v: " << field_value(subset_index,DISPLACEMENT_Y)
        << " theta: " << field_value(subset_index,ROTATION_Z) << " sigma: " << field_value(subset_index,SIGMA) << " gamma: " << field_value(subset_index,GAMMA));
    }
  }

  // compute post-processed quantities
  // For now, this assumes that all the fields are synched so that everyone owns all values.
  // TODO In the future, this can be parallelized
  // Complete the set up activities for the post processors
  // TODO maybe only processor 0 does this
  for(size_t i=0;i<post_processors_.size();++i){
    post_processors_[i]->execute();
  }
  update_image_frame();
};

bool
Schema::motion_detected(const int_t subset_gid){
  if(motion_window_params_->find(subset_gid)!=motion_window_params_->end()){
    const int_t use_subset_id = motion_window_params_->find(subset_gid)->second.use_subset_id_==-1 ? subset_gid:
        motion_window_params_->find(subset_gid)->second.use_subset_id_;
    if(motion_detectors_[use_subset_id]==Teuchos::null){
      // create the motion detector because it doesn't exist
      Motion_Window_Params mwp = motion_window_params_->find(use_subset_id)->second;
      motion_detectors_[use_subset_id] = Teuchos::rcp(new Motion_Test_Initializer(mwp.origin_x_,
        mwp.origin_y_,
        mwp.width_,
        mwp.height_,
        mwp.tol_));
    }
    TEUCHOS_TEST_FOR_EXCEPTION(motion_detectors_[use_subset_id]==Teuchos::null,std::runtime_error,
      "Error, the motion detector should exist here, but it doesn't.");
    bool motion_det = motion_detectors_[use_subset_id]->motion_detected(def_img_);
    DEBUG_MSG("Subset " << subset_gid << " TEST_FOR_MOTION using window defined for subset " << use_subset_id <<
      " result " << motion_det);
    return motion_det;
  }
  else{
    DEBUG_MSG("Subset " << subset_gid << " will not test for motion");
    return true;
  }
}

void
Schema::record_failed_step(const int_t subset_gid,
  const int_t status,
  const int_t num_iterations){
  local_field_value(subset_gid,SIGMA) = -1.0;
  local_field_value(subset_gid,MATCH) = -1.0;
  local_field_value(subset_gid,GAMMA) = -1.0;
  local_field_value(subset_gid,STATUS_FLAG) = status;
  local_field_value(subset_gid,ITERATIONS) = num_iterations;
}

void
Schema::record_step(const int_t subset_gid,
  Teuchos::RCP<std::vector<scalar_t> > & deformation,
  const scalar_t & sigma,
  const scalar_t & match,
  const scalar_t & gamma,
  const int_t status,
  const int_t num_iterations){
  local_field_value(subset_gid,DISPLACEMENT_X) = (*deformation)[DISPLACEMENT_X];
  local_field_value(subset_gid,DISPLACEMENT_Y) = (*deformation)[DISPLACEMENT_Y];
  local_field_value(subset_gid,NORMAL_STRAIN_X) = (*deformation)[NORMAL_STRAIN_X];
  local_field_value(subset_gid,NORMAL_STRAIN_Y) = (*deformation)[NORMAL_STRAIN_Y];
  local_field_value(subset_gid,SHEAR_STRAIN_XY) = (*deformation)[SHEAR_STRAIN_XY];
  local_field_value(subset_gid,ROTATION_Z) = (*deformation)[DICe::ROTATION_Z];
  local_field_value(subset_gid,SIGMA) = sigma;
  local_field_value(subset_gid,MATCH) = match; // 0 means data is successful
  local_field_value(subset_gid,GAMMA) = gamma;
  local_field_value(subset_gid,STATUS_FLAG) = status;
  local_field_value(subset_gid,ITERATIONS) = num_iterations;
}

void
Schema::generic_correlation_routine(Teuchos::RCP<Objective> obj){

  const int_t subset_gid = obj->correlation_point_global_id();
  assert(get_local_id(subset_gid)!=-1 && "Error: subset id is not local to this process.");
  DEBUG_MSG("[PROC " << comm_->get_rank() << "] SUBSET " << subset_gid << " (" << local_field_value(subset_gid,DICe::COORDINATE_X) <<
    "," << local_field_value(subset_gid,DICe::COORDINATE_Y) << ")");
  //
  //  test for motion if requested by the user in the subsets.txt file
  //
  bool motion = motion_detected(subset_gid);
  if(!motion){
    DEBUG_MSG("Subset " << subset_gid << " skipping frame due to no motion");
    // only change the match value and the status flag
    local_field_value(subset_gid,MATCH) = 0.0;
    local_field_value(subset_gid,STATUS_FLAG) = static_cast<int_t>(FRAME_SKIPPED_DUE_TO_NO_MOTION);
    local_field_value(subset_gid,ITERATIONS) = 0;
    return;
  }
  //
  //  check if the user has specified a path file for this subset:
  //  Path files help with defining an expected trajectory, can be used to initialize
  //  at any random time in a video sequence or to test if the computed solution is too far
  //  from the expected path to be valid
  //
  const bool has_path_file = path_file_names_->find(subset_gid)!=path_file_names_->end();
  bool global_path_search_required = local_field_value(subset_gid,SIGMA)==-1.0 || image_frame_==0;
  if(opt_initializers_[subset_gid]==Teuchos::null){
    if(has_path_file){
      const int_t num_neighbors = 6; // number of path neighbors to search while initializing
      std::string path_file_name = path_file_names_->find(subset_gid)->second;
      DEBUG_MSG("Subset " << subset_gid << " using path file " << path_file_name);
      opt_initializers_[subset_gid] =
          Teuchos::rcp(new Path_Initializer(obj->subset(),path_file_name.c_str(),num_neighbors));
    }
    else{
      DEBUG_MSG("Subset " << subset_gid << " no path file specified for this subset");
    }
  }
  TEUCHOS_TEST_FOR_EXCEPTION(opt_initializers_[subset_gid]==
      Teuchos::null&&has_path_file,std::runtime_error,"Initializer not instantiated yet, but should be.");
  //
  //  initialial guess for the subset's solution parameters
  //
  Status_Flag init_status = INITIALIZE_SUCCESSFUL;
  Status_Flag corr_status = CORRELATION_FAILED;
  int_t num_iterations = -1;
  scalar_t initial_gamma = 0.0;
  Teuchos::RCP<std::vector<scalar_t> > deformation = Teuchos::rcp(new std::vector<scalar_t>(DICE_DEFORMATION_SIZE,0.0));
  try{
    // use the path file to get the initial guess
    if(has_path_file){
      if(global_path_search_required){
        initial_gamma = opt_initializers_[subset_gid]->initial_guess(def_img_,deformation);
      }
      else{
        const scalar_t prev_u = local_field_value(subset_gid,DICe::DISPLACEMENT_X);
        const scalar_t prev_v = local_field_value(subset_gid,DICe::DISPLACEMENT_Y);
        const scalar_t prev_t = local_field_value(subset_gid,DICe::ROTATION_Z);
        initial_gamma = opt_initializers_[subset_gid]->initial_guess(def_img_,deformation,prev_u,prev_v,prev_t);
      }
    }
    // use the solution in the field values as the initial guess
    else if(initialization_method_==DICe::USE_FIELD_VALUES ||
        (initialization_method_==DICe::USE_NEIGHBOR_VALUES_FIRST_STEP_ONLY && image_frame_>0)){
      init_status = obj->initialize_from_previous_frame(deformation);
    }
    // use phase correlation of the whole image to get the initial guess
    // (useful when the subset being tracked is on an object translating through the frame)
    else if(initialization_method_==DICe::USE_PHASE_CORRELATION){
      (*deformation)[DISPLACEMENT_X] = phase_cor_u_x_ + local_field_value(subset_gid,DISPLACEMENT_X);
      (*deformation)[DISPLACEMENT_Y] = phase_cor_u_y_ + local_field_value(subset_gid,DISPLACEMENT_Y);
      (*deformation)[ROTATION_Z] = local_field_value(subset_gid,ROTATION_Z);
    }
    // use the solution of a neighboring subset for the intial guess
    else{
      init_status = obj->initialize_from_neighbor(deformation);
    }
  }
  catch (std::logic_error &err) { // a non-graceful exception occurred in initialization
    record_failed_step(subset_gid,static_cast<int_t>(INITIALIZE_FAILED_BY_EXCEPTION),num_iterations);
    return;
  };
  //
  //  check if initialization was successful
  //
  if(init_status==INITIALIZE_FAILED){
    record_failed_step(subset_gid,static_cast<int_t>(init_status),num_iterations);
    return;
  }
  //
  //  check if the user rrequested to skip the solve and only initialize (param set in subset file)
  //
  // check if the solve should be skipped
  if(skip_solve_flags_->find(subset_gid)!=skip_solve_flags_->end()){
    if(skip_solve_flags_->find(subset_gid)->second ==true){
      DEBUG_MSG("Subset " << subset_gid << " solve will be skipped as requested by user in the subset file");
      const scalar_t initial_sigma = obj->sigma(deformation);
      if(initial_gamma==0.0) initial_gamma = obj->gamma(deformation);
      record_step(subset_gid,deformation,initial_sigma,0.0,initial_gamma,static_cast<int_t>(FRAME_SKIPPED),num_iterations);
      return;
    }
  }
  //
  //  if user requested testing the initial value of gamma, do that here
  //
  if(initial_gamma_threshold_!=-1.0&&initial_gamma > initial_gamma_threshold_){
    DEBUG_MSG("Subset " << subset_gid << " initial gamma value FAILS threshold test, gamma: " <<
      initial_gamma << " (threshold: " << initial_gamma_threshold_ << ")");
    record_failed_step(subset_gid,static_cast<int_t>(INITIALIZE_FAILED),num_iterations);
    return;
  }
  // TODO how to respond to bad initialization
  // TODO add a search-based method to initialize if other methods failed
  //
  // perform the correlation
  //
  if(optimization_method_==DICe::GRADIENT_BASED||optimization_method_==DICe::GRADIENT_BASED_THEN_SIMPLEX){
    try{
      corr_status = obj->computeUpdateFast(deformation,num_iterations);
    }
    catch (std::logic_error &err) { //a non-graceful exception occurred
      corr_status = CORRELATION_FAILED_BY_EXCEPTION;
    };
  }
  else if(optimization_method_==DICe::SIMPLEX||optimization_method_==DICe::SIMPLEX_THEN_GRADIENT_BASED){
    try{
      corr_status = obj->computeUpdateRobust(deformation,num_iterations);
    }
    catch (std::logic_error &err) { //a non-graceful exception occurred
      corr_status = CORRELATION_FAILED_BY_EXCEPTION;
    };
  }
  if(corr_status!=CORRELATION_SUCCESSFUL){
    if(optimization_method_==DICe::SIMPLEX||optimization_method_==DICe::GRADIENT_BASED){
      record_failed_step(subset_gid,static_cast<int_t>(corr_status),num_iterations);
      return;
    }
    else if(optimization_method_==DICe::GRADIENT_BASED_THEN_SIMPLEX){
      // try again using simplex
      if(initialization_method_==DICe::USE_FIELD_VALUES || (initialization_method_==DICe::USE_NEIGHBOR_VALUES_FIRST_STEP_ONLY && image_frame_>0))
        init_status = obj->initialize_from_previous_frame(deformation);
      else if(initialization_method_==DICe::USE_PHASE_CORRELATION){
        (*deformation)[DISPLACEMENT_X] = phase_cor_u_x_ + local_field_value(subset_gid,DISPLACEMENT_X);
        (*deformation)[DISPLACEMENT_Y] = phase_cor_u_y_ + local_field_value(subset_gid,DISPLACEMENT_Y);
        (*deformation)[ROTATION_Z] = local_field_value(subset_gid,ROTATION_Z);
        // TODO clear the other values?
        init_status = DICe::INITIALIZE_SUCCESSFUL;
      }
      else init_status = obj->initialize_from_neighbor(deformation);
      try{
        corr_status = obj->computeUpdateRobust(deformation,num_iterations);
      }
      catch (std::logic_error &err) { //a non-graceful exception occurred
        corr_status = CORRELATION_FAILED_BY_EXCEPTION;
      };
      if(corr_status!=CORRELATION_SUCCESSFUL){
        record_failed_step(subset_gid,static_cast<int_t>(corr_status),num_iterations);
        return;
      }
    }
    else if(optimization_method_==DICe::SIMPLEX_THEN_GRADIENT_BASED){
      // try again using gradient based
      if(initialization_method_==DICe::USE_FIELD_VALUES || (initialization_method_==DICe::USE_NEIGHBOR_VALUES_FIRST_STEP_ONLY && image_frame_>0))
        init_status = obj->initialize_from_previous_frame(deformation);
      else if(initialization_method_==DICe::USE_PHASE_CORRELATION){
        (*deformation)[DISPLACEMENT_X] = phase_cor_u_x_ + local_field_value(subset_gid,DISPLACEMENT_X);
        (*deformation)[DISPLACEMENT_Y] = phase_cor_u_y_ + local_field_value(subset_gid,DISPLACEMENT_Y);
        (*deformation)[ROTATION_Z] = local_field_value(subset_gid,ROTATION_Z);
        // TODO clear the other values?
        init_status = DICe::INITIALIZE_SUCCESSFUL;
      }
      else init_status = obj->initialize_from_neighbor(deformation);
      try{
        corr_status = obj->computeUpdateFast(deformation,num_iterations);
      }
      catch (std::logic_error &err) { //a non-graceful exception occurred
        corr_status = CORRELATION_FAILED_BY_EXCEPTION;
      };
      if(corr_status!=CORRELATION_SUCCESSFUL){
        record_failed_step(subset_gid,static_cast<int_t>(corr_status),num_iterations);
        return;
      }
    }
  }
  //
  //  test final gamma if user requested
  //
  const scalar_t gamma = obj->gamma(deformation);
  const scalar_t sigma = obj->sigma(deformation);
  if(final_gamma_threshold_!=-1.0&&gamma > final_gamma_threshold_){
    DEBUG_MSG("Subset " << subset_gid << " final gamma value FAILS threshold test, gamma: " <<
      gamma << " (threshold: " << final_gamma_threshold_ << ")");
    // for the phase correlation initialization method, the initial guess needs to be stored
    if(initialization_method_==DICe::USE_PHASE_CORRELATION){
      local_field_value(subset_gid,DISPLACEMENT_X) += phase_cor_u_x_;
      local_field_value(subset_gid,DISPLACEMENT_Y) += phase_cor_u_y_;
    }
    record_failed_step(subset_gid,static_cast<int_t>(FRAME_FAILED_DUE_TO_HIGH_GAMMA),num_iterations);
    return;
  }
  //
  //  test path distance if user requested
  //
  if(path_distance_threshold_!=-1.0&&has_path_file){
    scalar_t path_distance = 0.0;
    size_t id = 0;
    // dynamic cast the pointer to get access to the derived class methods
    Teuchos::RCP<Path_Initializer> path_initializer =
        Teuchos::rcp_dynamic_cast<Path_Initializer>(opt_initializers_[subset_gid]);
    path_initializer->closest_triad((*deformation)[DISPLACEMENT_X],
      (*deformation)[DISPLACEMENT_Y],(*deformation)[ROTATION_Z],id,path_distance);
    DEBUG_MSG("Subset " << subset_gid << " path distance: " << path_distance);
    if(path_distance > path_distance_threshold_)
    {
      DEBUG_MSG("Subset " << subset_gid << " path distance value FAILS threshold test, distance from path: " <<
        path_distance << " (threshold: " << path_distance_threshold_ << ")");
      record_failed_step(subset_gid,static_cast<int_t>(FRAME_FAILED_DUE_TO_HIGH_PATH_DISTANCE),num_iterations);
      return;
    }
  }
  // TODO how to respond to failure here? or for initialization?
  //
  // SUCCESS
  //
  if(projection_method_==VELOCITY_BASED) save_off_fields(subset_gid);
  record_step(subset_gid,deformation,sigma,0.0,gamma,static_cast<int_t>(init_status),num_iterations);
  //
  //  turn on pixels that at the beginning were hidden behind an obstruction
  //
  if(use_subset_evolution_&&image_frame_>1){
    DEBUG_MSG("[PROC " << comm_->get_rank() << "] Evolving subset " << subset_gid << " using newly exposed pixels for intensity values");
    obj->subset()->turn_on_previously_obstructed_pixels();
  }
  //
  //  Write debugging images if requested
  //
  if(output_deformed_subset_intensity_images_){
#ifndef DICE_DISABLE_BOOST_FILESYSTEM
    DEBUG_MSG("[PROC " << comm_->get_rank() << "] Attempting to create directory : ./deformed_subset_intensities/");
    std::string dirStr = "./deformed_subset_intensities/";
    boost::filesystem::path dir(dirStr);
    if(boost::filesystem::create_directory(dir)) {
      DEBUG_MSG("[PROC " << comm_->get_rank() << "] Directory successfully created");
    }
    int_t num_zeros = 0;
    if(num_image_frames_>0){
      int_t num_digits_total = 0;
      int_t num_digits_image = 0;
      int_t decrement_total = num_image_frames_;
      int_t decrement_image = image_frame_;
      while (decrement_total){decrement_total /= 10; num_digits_total++;}
      if(image_frame_==0) num_digits_image = 1;
      else
        while (decrement_image){decrement_image /= 10; num_digits_image++;}
      num_zeros = num_digits_total - num_digits_image;
    }
    std::stringstream ss;
    ss << dirStr << "deformedSubset_" << subset_gid << "_";
    for(int_t i=0;i<num_zeros;++i)
      ss << "0";
    ss << image_frame_;
    obj->subset()->write_tiff(ss.str(),true);
#endif
  }
  if(output_evolved_subset_images_){
#ifndef DICE_DISABLE_BOOST_FILESYSTEM
    DEBUG_MSG("[PROC " << comm_->get_rank() << "] Attempting to create directory : ./evolved_subsets/");
    std::string dirStr = "./evolved_subsets/";
    boost::filesystem::path dir(dirStr);
    if(boost::filesystem::create_directory(dir)) {
      DEBUG_MSG("[PROC " << comm_->get_rank() << "[ Directory successfully created");
    }
    int_t num_zeros = 0;
    if(num_image_frames_>0){
      int_t num_digits_total = 0;
      int_t num_digits_image = 0;
      int_t decrement_total = num_image_frames_;
      int_t decrement_image = image_frame_;
      while (decrement_total){decrement_total /= 10; num_digits_total++;}
      if(image_frame_==0) num_digits_image = 1;
      else
        while (decrement_image){decrement_image /= 10; num_digits_image++;}
      num_zeros = num_digits_total - num_digits_image;
    }
    std::stringstream ss;
    ss << dirStr << "evolvedSubset_" << subset_gid << "_";
    for(int_t i=0;i<num_zeros;++i)
      ss << "0";
    ss << image_frame_;
    obj->subset()->write_tiff(ss.str());
#endif
  }
}

// TODO fix this up so that it works with conformal subsets:
void
Schema::write_control_points_image(const std::string & fileName,
  const bool use_def_image,
  const bool use_one_point){

  assert(subset_dim_>0);
  Teuchos::RCP<Image> img = (use_def_image) ? def_img_ : ref_img_;

  const int_t width = img->width();
  const int_t height = img->height();

  // first, create new intensities based on the old
  Teuchos::ArrayRCP<intensity_t> intensities(width*height,0.0);
  Teuchos::ArrayRCP<intensity_t> img_intensity_values = img->intensity_array();
  for (int_t i=0;i<width*height;++i)
    intensities[i] = img_intensity_values[i];

  int_t x=0,y=0,xAlt=0,yAlt=0;
  const int_t numLocalControlPts = data_num_points_; //cp_map_->getNodeNumElements();
  // put a black box around the subset
  int_t i_start = 0;
  if(use_one_point) i_start = numLocalControlPts/2;
  const int_t i_end = use_one_point ? i_start + 1 : numLocalControlPts;
  const int_t color = use_one_point ? 255 : 0;
  for (int_t i=i_start;i<i_end;++i){
    x = field_value(i,COORDINATE_X);
    y = field_value(i,COORDINATE_Y);
    for(int_t j=0;j<subset_dim_;++j){
      xAlt = x - subset_dim_/2 + j;
      intensities[(y+subset_dim_/2)*width+xAlt] = color;
      intensities[(y-subset_dim_/2)*width+xAlt] = color;
    }
    for(int_t j=0;j<subset_dim_;++j){
      yAlt = y - subset_dim_/2 + j;
      intensities[(x+subset_dim_/2)+width*yAlt] = color;
      intensities[(x-subset_dim_/2)+width*yAlt] = color;
    }
  }
  // place white plus signs at the control points
  for (int_t i=0;i<numLocalControlPts;++i){
    x = field_value(i,COORDINATE_X);
    y = field_value(i,COORDINATE_Y);
    intensities[y*width+x] = 255;
    for(int_t j=0;j<3;++j){
      intensities[y*width+(x+j)] = 255;
      intensities[y*width+(x-j)] = 255;
      intensities[(y+j)*width + x] = 255;
      intensities[(y-j)*width + x] = 255;
    }
  }
  // place black plus signs at the control points that were successful
  for (int_t i=0;i<numLocalControlPts;++i){
    if(field_value(i,SIGMA)<=0) return;
    x = field_value(i,COORDINATE_X);
    y = field_value(i,COORDINATE_Y);
    intensities[y*width+x] = 0;
    for(int_t j=0;j<2;++j){
      intensities[y*width+(x+j)] = 0;
      intensities[y*width+(x-j)] = 0;
      intensities[(y+j)*width + x] = 0;
      intensities[(y-j)*width + x] = 0;
    }
  }
  // create a new image based on the info above:
  Teuchos::RCP<Image> new_img = Teuchos::rcp(new Image(width,height,intensities));

  // write the image:
  new_img->write_tiff(fileName);
}

void
Schema::write_output(const std::string & output_folder,
  const std::string & prefix,
  const bool separate_files_per_subset, const Output_File_Type type){
//  assert(analysis_type_!=CONSTRAINED_OPT && "Error, writing output from a schema using constrained optimization is not enabled.");
//  assert(analysis_type_!=INTEGRATED_DIC && "Error, writing output from a schema using integrated DIC is not enabled.");
  int_t my_proc = comm_->get_rank();
  if(my_proc!=0) return;
  int_t proc_size = comm_->get_size();

  assert(type==TEXT_FILE && "Currently only TEXT_FILE output is implemented");
  assert(output_spec_!=Teuchos::null);

  if(separate_files_per_subset){
    for(int_t subset=0;subset<data_num_points_;++subset){
      // determine the number of digits to append:
      int_t num_digits_total = 0;
      int_t num_digits_subset = 0;
      int_t decrement_total = data_num_points_;
      int_t decrement_subset = subset;
      while (decrement_total){decrement_total /= 10; num_digits_total++;}
      if(subset==0) num_digits_subset = 1;
      else
        while (decrement_subset){decrement_subset /= 10; num_digits_subset++;}
      int_t num_zeros = num_digits_total - num_digits_subset;

      // determine the file name for this subset
      std::stringstream fName;
      fName << output_folder << prefix << "_";
      for(int_t i=0;i<num_zeros;++i)
        fName << "0";
      fName << subset;
      if(proc_size>1)
        fName << "." << proc_size;
      fName << ".txt";
      if(image_frame_==1){
        std::FILE * filePtr = fopen(fName.str().c_str(),"w"); // overwrite the file if it exists
        output_spec_->write_header(filePtr,"FRAME");
        fclose (filePtr);
      }
      // append the latest result to the file
      std::FILE * filePtr = fopen(fName.str().c_str(),"a");
      output_spec_->write_frame(filePtr,image_frame_,subset);
      fclose (filePtr);
    } // subset loop
  }
  else{
    std::stringstream fName;
    // determine the number of digits to append:
    int_t num_zeros = 0;
    if(num_image_frames_>0){
      int_t num_digits_total = 0;
      int_t num_digits_image = 0;
      int_t decrement_total = num_image_frames_;
      int_t decrement_image = image_frame_ -1;
      while (decrement_total){decrement_total /= 10; num_digits_total++;}
      if(image_frame_-1==0) num_digits_image = 1;
      else
        while (decrement_image){decrement_image /= 10; num_digits_image++;}
      num_zeros = num_digits_total - num_digits_image;
    }
    fName << output_folder << prefix << "_";
    for(int_t i=0;i<num_zeros;++i)
      fName << "0";
    fName << image_frame_ - 1;
    if(proc_size >1)
      fName << "." << proc_size;
    fName << ".txt";
    std::FILE * filePtr = fopen(fName.str().c_str(),"w");
    output_spec_->write_header(filePtr,"SUBSET_ID");
    for(int_t i=0;i<data_num_points_;++i){
      output_spec_->write_frame(filePtr,i,i);
    }
    fclose (filePtr);
  }
}

void
Schema::print_fields(const std::string & fileName){

  if(data_num_points_==0){
    std::cout << " Schema has 0 control points." << std::endl;
    return;
  }
  if(fields_->get_num_fields()==0){
    std::cout << " Schema fields are emplty." << std::endl;
    return;
  }
  const int_t proc_id = comm_->get_rank();

  if(fileName==""){
    std::cout << "[PROC " << proc_id << "] DICE::Schema Fields and Values: " << std::endl;
    for(int_t i=0;i<data_num_points_;++i){
      std::cout << "[PROC " << proc_id << "] Control Point ID: " << i << std::endl;
      for(int_t j=0;j<DICe::MAX_FIELD_NAME;++j){
        std::cout << "[PROC " << proc_id << "]   " << to_string(static_cast<Field_Name>(j)) <<  " " <<
            field_value(i,static_cast<Field_Name>(j)) << std::endl;
        if(dist_map_->get_local_element(i)!=-1){
          std::cout << "[PROC " << proc_id << "]   " << to_string(static_cast<Field_Name>(j)) <<  " (has distributed value)  " <<
              local_field_value(i,static_cast<Field_Name>(j)) << std::endl;
        }
      }
    }

  }
  else{
    std::FILE * outFile;
    outFile = fopen(fileName.c_str(),"a");
    for(int_t i=0;i<data_num_points_;++i){
      fprintf(outFile,"%i ",i);
      for(int_t j=0;j<DICe::MAX_FIELD_NAME;++j){
        fprintf(outFile," %4.4E ",field_value(i,static_cast<Field_Name>(j)));
      }
      fprintf(outFile,"\n");
    }
    fclose(outFile);
  }
}

void
Schema::check_for_blocking_subsets(const int_t subset_global_id){
  if(obstructing_subset_ids_==Teuchos::null) return;
  if(obstructing_subset_ids_->find(subset_global_id)==obstructing_subset_ids_->end()) return;
  if(obstructing_subset_ids_->find(subset_global_id)->second.size()==0) return;

  const int_t subset_local_id = get_local_id(subset_global_id);

  // turn off pixels in subset 0 that are blocked by 1 and 2
  // get a pointer to the member data in the subset that will store the list of blocked pixels
  std::set<std::pair<int_t,int_t> > & blocked_pixels =
      *obj_vec_[subset_local_id]->subset()->pixels_blocked_by_other_subsets();
  blocked_pixels.clear();

  // get the list of subsets that block this one
  std::vector<int_t> * obst_ids = &obstructing_subset_ids_->find(subset_global_id)->second;
  // iterate over all the blocking subsets
  for(size_t si=0;si<obst_ids->size();++si){
    int_t global_ss = (*obst_ids)[si];
    int_t local_ss = get_local_id(global_ss);
    assert(local_ss>=0);
    int_t cx = obj_vec_[local_ss]->subset()->centroid_x();
    int_t cy = obj_vec_[local_ss]->subset()->centroid_y();
    Teuchos::RCP<std::vector<scalar_t> > def = Teuchos::rcp(new std::vector<scalar_t>(DICE_DEFORMATION_SIZE,0.0));
    (*def)[DICe::DISPLACEMENT_X]  = local_field_value(global_ss,DICe::DISPLACEMENT_X);
    (*def)[DICe::DISPLACEMENT_Y]  = local_field_value(global_ss,DICe::DISPLACEMENT_Y);
    (*def)[DICe::ROTATION_Z]      = local_field_value(global_ss,DICe::ROTATION_Z);
    (*def)[DICe::NORMAL_STRAIN_X] = local_field_value(global_ss,DICe::NORMAL_STRAIN_X);
    (*def)[DICe::NORMAL_STRAIN_Y] = local_field_value(global_ss,DICe::NORMAL_STRAIN_Y);
    (*def)[DICe::SHEAR_STRAIN_XY] = local_field_value(global_ss,DICe::SHEAR_STRAIN_XY);
    std::set<std::pair<int_t,int_t> > subset_pixels =
        obj_vec_[local_ss]->subset()->deformed_shapes(def,cx,cy,obstruction_skin_factor_);
    blocked_pixels.insert(subset_pixels.begin(),subset_pixels.end());
  } // blocking subsets loop
}

void
Schema::write_deformed_subsets_image(const bool use_gamma_as_color){
#ifndef DICE_DISABLE_BOOST_FILESYSTEM
  if(obj_vec_.empty()) return;
  // if the subset_images folder does not exist, create it
  // TODO allow user to specify where this goes
  // If the dir is already there this step becomes a no-op
  DEBUG_MSG("Attempting to create directory : ./deformed_subsets/");
  std::string dirStr = "./deformed_subsets/";
  boost::filesystem::path dir(dirStr);
  if(boost::filesystem::create_directory(dir)) {
    DEBUG_MSG("Directory successfully created");
  }

  int_t num_zeros = 0;
  if(num_image_frames_>0){
    int_t num_digits_total = 0;
    int_t num_digits_image = 0;
    int_t decrement_total = num_image_frames_;
    int_t decrement_image = image_frame_;
    while (decrement_total){decrement_total /= 10; num_digits_total++;}
    if(image_frame_==0) num_digits_image = 1;
    else
      while (decrement_image){decrement_image /= 10; num_digits_image++;}
    num_zeros = num_digits_total - num_digits_image;
  }
  const int_t proc_id = comm_->get_rank();
  std::stringstream ss;
  ss << dirStr << "def_subsets_p_" << proc_id << "_";
  for(int_t i=0;i<num_zeros;++i)
    ss << "0";
  ss << image_frame_ << ".tif";

  // construct a copy of the base image to use as layer 0 for the output;

  const int_t w = def_img_->width();
  const int_t h = def_img_->height();

  Teuchos::ArrayRCP<intensity_t> intensities = def_img_->intensity_array();

  scalar_t dx=0,dy=0;
  int_t ox=0,oy=0;
  int_t Dx=0,Dy=0;
  scalar_t X=0.0,Y=0.0;
  int_t px=0,py=0;

  // create output for each subset
  //for(int_t subset=0;subset<1;++subset){
  for(size_t subset=0;subset<obj_vec_.size();++subset){
    const int_t gid = obj_vec_[subset]->correlation_point_global_id();
    //if(gid==1) continue;
    // get the deformation vector for each subset
    const scalar_t u     = local_field_value(gid,DICe::DISPLACEMENT_X);
    const scalar_t v     = local_field_value(gid,DICe::DISPLACEMENT_Y);
    const scalar_t theta = local_field_value(gid,DICe::ROTATION_Z);
    const scalar_t dudx  = local_field_value(gid,DICe::NORMAL_STRAIN_X);
    const scalar_t dvdy  = local_field_value(gid,DICe::NORMAL_STRAIN_Y);
    const scalar_t gxy   = local_field_value(gid,DICe::SHEAR_STRAIN_XY);
    DEBUG_MSG("Write deformed subset " << gid << " u " << u << " v " << v << " theta " << theta << " dudx " << dudx << " dvdy " << dvdy << " gxy " << gxy);
    Teuchos::RCP<DICe::Subset> ref_subset = obj_vec_[subset]->subset();
    ox = ref_subset->centroid_x();
    oy = ref_subset->centroid_y();
    scalar_t mean_sum_ref = 0.0;
    scalar_t mean_sum_def = 0.0;
    scalar_t mean_ref = 0.0;
    scalar_t mean_def = 0.0;
    if(use_gamma_as_color){
      mean_ref = obj_vec_[subset]->subset()->mean(REF_INTENSITIES,mean_sum_ref);
      mean_def = obj_vec_[subset]->subset()->mean(DEF_INTENSITIES,mean_sum_def);
      TEUCHOS_TEST_FOR_EXCEPTION(mean_sum_ref==0.0||mean_sum_def==0.0,std::runtime_error," invalid mean sum (cannot be 0.0, ZNSSD is then undefined)" <<
        mean_sum_ref << " " << mean_sum_def);
    }
    // loop over each pixel in the subset
    scalar_t pixel_gamma = 0.0;
    for(int_t i=0;i<ref_subset->num_pixels();++i){
      dx = ref_subset->x(i) - ox;
      dy = ref_subset->y(i) - oy;
      // stretch and shear the coordinate
      Dx = (1.0+dudx)*dx + gxy*dy;
      Dy = (1.0+dvdy)*dy + gxy*dx;
      //  Rotation                                  // translation // convert to global coordinates
      X = std::cos(theta)*Dx - std::sin(theta)*Dy + u            + ox;
      Y = std::sin(theta)*Dx + std::cos(theta)*Dy + v            + oy;
      // get the nearest pixel location:
      px = (int_t)X;
      if(X - (int_t)X >= 0.5) px++;
      py = (int_t)Y;
      if(Y - (int_t)Y >= 0.5) py++;
      if(px>=0&&px<w&&py>=0&&py<h){
        if(use_gamma_as_color){
          if(ref_subset->is_active(i)&!ref_subset->is_deactivated_this_step(i)){
            pixel_gamma =  (ref_subset->def_intensities(i)-mean_def)/mean_sum_def - (ref_subset->ref_intensities(i)-mean_ref)/mean_sum_ref;
            intensities[py*w+px] = pixel_gamma*pixel_gamma*10000.0;
          }
        }else{
          if(!ref_subset->is_active(i)){
            intensities[py*w+px] = 75;
          }
          else{
            // color shows correlation quality
            intensities[py*w+px] = 100;//ref_subset->per_pixel_gamma(i)*85000;
          }
          // trun all deactivated pixels white
          if(ref_subset->is_deactivated_this_step(i)){
            intensities[py*w+px] = 255;
          }
        } // not use_gamma_as_color
      } // range guard
    } // pixel loop

  } // subset loop

  Teuchos::RCP<Image> layer_0_image = Teuchos::rcp(new Image(w,h,intensities));
  layer_0_image->write_tiff(ss.str());
#else
  DEBUG_MSG("Warning, write_deformed_image() was called, but Boost::filesystem is not enabled making this a no-op.");
#endif
}


int_t
Schema::strain_window_size(const int_t post_processor_index)const{
  assert((int_t)post_processors_.size()>post_processor_index);
    return post_processors_[post_processor_index]->strain_window_size();
}

Output_Spec::Output_Spec(Schema * schema,
  const bool omit_row_id,
  const Teuchos::RCP<Teuchos::ParameterList> & params,
  const std::string & delimiter):
  schema_(schema),
  delimiter_(delimiter), // space delimited is default
  omit_row_id_(omit_row_id)
{

  // default output format
  if(params == Teuchos::null){
    field_names_.push_back(to_string(DICe::COORDINATE_X));
    post_processor_ids_.push_back(-1);
    field_names_.push_back(to_string(DICe::COORDINATE_Y));
    post_processor_ids_.push_back(-1);
    field_names_.push_back(to_string(DICe::DISPLACEMENT_X));
    post_processor_ids_.push_back(-1);
    field_names_.push_back(to_string(DICe::DISPLACEMENT_Y));
    post_processor_ids_.push_back(-1);
    field_names_.push_back(to_string(DICe::ROTATION_Z));
    post_processor_ids_.push_back(-1);
    field_names_.push_back(to_string(DICe::NORMAL_STRAIN_X));
    post_processor_ids_.push_back(-1);
    field_names_.push_back(to_string(DICe::NORMAL_STRAIN_Y));
    post_processor_ids_.push_back(-1);
    field_names_.push_back(to_string(DICe::SHEAR_STRAIN_XY));
    post_processor_ids_.push_back(-1);
    field_names_.push_back(to_string(DICe::SIGMA));
    post_processor_ids_.push_back(-1);
    field_names_.push_back(to_string(DICe::STATUS_FLAG));
    post_processor_ids_.push_back(-1);
  }
  else{
    // get the total number of field names
    const int_t num_names = params->numParams();
    // get the max index
    field_names_.resize(num_names);
    post_processor_ids_.resize(num_names);
    int_t max_index = 0;
    std::set<int_t> indices;

    // read in the names and indices by iterating the parameter list
    for(Teuchos::ParameterList::ConstIterator it=params->begin();it!=params->end();++it){
      std::string string_field_name = it->first;
      stringToUpper(string_field_name);
      int_t post_processor_id = -1;
      bool paramValid = false;
      for(int_t j=0;j<MAX_FIELD_NAME;++j){
        if(string_field_name==to_string(static_cast<Field_Name>(j)))
          paramValid = true;
      }
      // see if this field is in one of the post processors instead
      for(int_t j=0;j<(int_t)schema_->post_processors()->size();++j){
        for(int_t k=0;k<(int_t)(*schema_->post_processors())[j]->field_names()->size();++k){
          if(string_field_name==(*(*schema_->post_processors())[j]->field_names())[k]){
            paramValid = true;
            post_processor_id = j;
          }
        }
      }
      if(!paramValid){
        std::cout << "Error: invalid field name requested in output spec: " << string_field_name << std::endl;
        assert(false);
      }
      const int_t field_index = params->get<int_t>(string_field_name);
      if(field_index>num_names-1||field_index<0){
        std::cout << "Error: field index in output spec is invalid " << field_index << std::endl;
        assert(false);
      }
      // see if this index exists already
      if(indices.find(field_index)!=indices.end()){
        std::cout << "Error: same field index assigned to multiple fields in output spec" << field_index << std::endl;
        assert(false);
      }
      indices.insert(field_index);
      if(field_index > max_index) max_index = field_index;
      field_names_[field_index] = string_field_name;
      post_processor_ids_[field_index] = post_processor_id;
    } // loop over field names in the parameterlist
    if(max_index!=num_names-1){
      std::cout << "Error: The max field index in the output spec is not equal to the number of fields, num_fields " << field_names_.size() << " max_index " << max_index << std::endl;
      assert(false);
    }
  }
};

void
Output_Spec::write_header(std::FILE * file,
  const std::string & row_id){
  assert(file);
  fprintf(file,"***\n");
  fprintf(file,"*** Digital Image Correlation Engine (DICe), Copyright 2015 Sandia Corporation\n");
  fprintf(file,"***\n");
  fprintf(file,"*** Reference image: %s \n",schema_->ref_img()->file_name().c_str());
  fprintf(file,"*** Deformed image: %s \n",schema_->def_img()->file_name().c_str());
  if(schema_->analysis_type()==GLOBAL_DIC){
    fprintf(file,"*** DIC method : global \n");
  }
  else{
    fprintf(file,"*** DIC method : local \n");
  }
  fprintf(file,"*** Correlation method: ZNSSD\n");
  std::string interp_method = to_string(schema_->interpolation_method());
  fprintf(file,"*** Interpolation method: %s\n",interp_method.c_str());
  fprintf(file,"*** Image gradient method: FINITE_DIFFERENCE\n");
  std::string opt_method = to_string(schema_->optimization_method());
  fprintf(file,"*** Optimization method: %s\n",opt_method.c_str());
  std::string proj_method = to_string(schema_->projection_method());
  fprintf(file,"*** Projection method: %s\n",proj_method.c_str());
  std::string init_method = to_string(schema_->initialization_method());
  fprintf(file,"*** Guess initialization method: %s\n",init_method.c_str());
  fprintf(file,"*** Seed location: N/A\n");
  fprintf(file,"*** Shape functions: ");
  if(schema_->translation_enabled())
    fprintf(file,"Translation (u,v) ");
  if(schema_->rotation_enabled())
    fprintf(file,"Rotation (theta) ");
  if(schema_->normal_strain_enabled())
    fprintf(file,"Normal Strain (ex,ey) ");
  if(schema_->shear_strain_enabled())
    fprintf(file,"Shear Strain (gamma_xy) ");
  fprintf(file,"\n");
  fprintf(file,"*** Incremental correlation: false\n");
  if(schema_->analysis_type()==GLOBAL_DIC){
    fprintf(file,"*** Mesh size: %i\n",schema_->mesh_size());
    fprintf(file,"*** Step size: N/A\n");
  }
  else{
    fprintf(file,"*** Subset size: %i\n",schema_->subset_dim());
    fprintf(file,"*** Step size: x %i y %i (-1 implies not regular grid)\n",schema_->step_size_x(),schema_->step_size_y());
  }
  if(schema_->post_processors()->size()==0)
    fprintf(file,"*** Strain window: N/A\n");
  else
    fprintf(file,"*** Strain window size in pixels: %i (only first strain post-processor is reported)\n",schema_->strain_window_size(0));
  fprintf(file,"*** Coordinates given with (0,0) as upper left corner of image, x positive right, y positive down\n");
  fprintf(file,"***\n");
  if(!omit_row_id_)
    fprintf(file,"%s%s",row_id.c_str(),delimiter_.c_str());
  for(size_t i=0;i<field_names_.size();++i){
    if(i==0)
      fprintf(file,"%s",field_names_[i].c_str());
    else
      fprintf(file,"%s%s",delimiter_.c_str(),field_names_[i].c_str());
  }
  fprintf(file,"\n");
}

void
Output_Spec::write_frame(std::FILE * file,
  const int_t row_index,
  const int_t field_value_index){

  assert(file);
  if(!omit_row_id_)
    fprintf(file,"%i%s",row_index,delimiter_.c_str());
  assert(field_names_.size()==post_processor_ids_.size());
  for(size_t i=0;i<field_names_.size();++i)
  {
    // if the field_name is from one of the schema fields, get the information from the schema
    scalar_t value = 0.0;
    if(post_processor_ids_[i]==-1)
      value = schema_->field_value(field_value_index,string_to_field_name(field_names_[i]));
    // otherwise the field must belong to a post processor
    else{
      assert(post_processor_ids_[i]>=0 && post_processor_ids_[i]<(int_t)schema_->post_processors()->size());
      value = (*schema_->post_processors())[post_processor_ids_[i]]->field_value(field_value_index,field_names_[i]);
    }
    if(i==0)
      fprintf(file,"%4.4E",value);
    else
      fprintf(file,"%s%4.4E",delimiter_.c_str(),value);
  }
  fprintf(file,"\n"); // the space before end of line is important for parsing in the output diff tool
}

}// End DICe Namespace