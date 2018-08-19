/*M///////////////////////////////////////////////////////////////////////////////////////
 //
 //  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
 //
 //  By downloading, copying, installing or using the software you agree to this license.
 //  If you do not agree to this license, do not download, install,
 //  copy or use the software.
 //
 //
 //                           License Agreement
 //                For Open Source Computer Vision Library
 //
 // Copyright (C) 2015, OpenCV Foundation, all rights reserved.
 // Third party copyrights are property of their respective owners.
 //
 // Redistribution and use in source and binary forms, with or without modification,
 // are permitted provided that the following conditions are met:
 //
 //   * Redistribution's of source code must retain the above copyright notice,
 //     this list of conditions and the following disclaimer.
 //
 //   * Redistribution's in binary form must reproduce the above copyright notice,
 //     this list of conditions and the following disclaimer in the documentation
 //     and/or other materials provided with the distribution.
 //
 //   * The name of the copyright holders may not be used to endorse or promote products
 //     derived from this software without specific prior written permission.
 //
 // This software is provided by the copyright holders and contributors "as is" and
 // any express or implied warranties, including, but not limited to, the implied
 // warranties of merchantability and fitness for a particular purpose are disclaimed.
 // In no event shall the Intel Corporation or contributors be liable for any direct,
 // indirect, incidental, special, exemplary, or consequential damages
 // (including, but not limited to, procurement of substitute goods or services;
 // loss of use, data, or profits; or business interruption) however caused
 // and on any theory of liability, whether in contract, strict liability,
 // or tort (including negligence or otherwise) arising in any way out of
 // the use of this software, even if advised of the possibility of such damage.
 //
 //M*/

#ifndef __LIBMV_CAPI__
#define __LIBMV_CAPI__

#include "libmv/logging/logging.h"

#include "libmv/correspondence/feature.h"
#include "libmv/correspondence/feature_matching.h"
#include "libmv/correspondence/matches.h"
#include "libmv/correspondence/nRobustViewMatching.h"

#include "libmv/simple_pipeline/bundle.h"
#include "libmv/simple_pipeline/camera_intrinsics.h"
#include "libmv/simple_pipeline/keyframe_selection.h"
#include "libmv/simple_pipeline/initialize_reconstruction.h"
#include "libmv/simple_pipeline/pipeline.h"
#include "libmv/simple_pipeline/reconstruction_scale.h"
#include "libmv/simple_pipeline/tracks.h"
#include <opencv2/xfeatures2d.hpp>

using namespace cv;
using namespace libmv;

////////////////////////////////////////
// Based on 'sfm' (opencv_contrib)
///////////////////////////////////////

/** @brief Data structure describing the camera model and its parameters.
  @param _distortion_model Type of camera model.
  @param _focal_length focal length of the camera (in pixels).
  @param _principal_point_x principal point of the camera in the x direction (in pixels).
  @param _principal_point_y principal point of the camera in the y direction (in pixels).
  @param _polynomial_k1 radial distortion parameter.
  @param _polynomial_k2 radial distortion parameter.
  @param _polynomial_k3 radial distortion parameter.
  @param _polynomial_p1 radial distortion parameter.
  @param _polynomial_p2 radial distortion parameter.

  Is assumed that modern cameras have their principal point in the image center.\n
  In case that the camera model was SFM_DISTORTION_MODEL_DIVISION, it's only needed to provide
  _polynomial_k1 and _polynomial_k2 which will be assigned as division distortion parameters.
 */
class CV_EXPORTS_W_SIMPLE libmv_CameraIntrinsicsOptions
{
public:
  CV_WRAP
  libmv_CameraIntrinsicsOptions(const int _distortion_model=0,
                                const double _focal_length_x=0,
                                const double _focal_length_y=0,
                                const double _principal_point_x=0,
                                const double _principal_point_y=0,
                                const double _polynomial_k1=0,
                                const double _polynomial_k2=0,
                                const double _polynomial_k3=0,
                                const double _polynomial_p1=0,
                                const double _polynomial_p2=0)
    : distortion_model(_distortion_model),
      image_width(2*_principal_point_x),
      image_height(2*_principal_point_y),
      focal_length_x(_focal_length_x),
      focal_length_y(_focal_length_y),
      principal_point_x(_principal_point_x),
      principal_point_y(_principal_point_y),
      polynomial_k1(_polynomial_k1),
      polynomial_k2(_polynomial_k2),
      polynomial_k3(_polynomial_k3),
      division_k1(_polynomial_p1),
      division_k2(_polynomial_p2)
  {
    if ( _distortion_model == libmv::DISTORTION_MODEL_DIVISION )
    {
      division_k1 = _polynomial_k1;
      division_k2 = _polynomial_k2;
    }
  }

  // Common settings of all distortion models.
  CV_PROP_RW int distortion_model;
  CV_PROP_RW int image_width, image_height;
  CV_PROP_RW double focal_length_x, focal_length_y;
  CV_PROP_RW double principal_point_x, principal_point_y;

  // Radial distortion model.
  CV_PROP_RW double polynomial_k1, polynomial_k2, polynomial_k3;
  CV_PROP_RW double polynomial_p1, polynomial_p2;

  // Division distortion model.
  CV_PROP_RW double division_k1, division_k2;
};

////////////////////////////////////////
// Based on 'sfm' (opencv_contrib)
///////////////////////////////////////

/** @brief Data structure describing the reconstruction options.
  @param _keyframe1 first keyframe used in order to initialize the reconstruction.
  @param _keyframe2 second keyframe used in order to initialize the reconstruction.
  @param _refine_intrinsics camera parameter or combination of parameters to refine.
  @param _select_keyframes allows to select automatically the initial keyframes. If 1 then autoselection is enabled. If 0 then is disabled.
  @param _verbosity_level verbosity logs level for Glog. If -1 then logs are disabled, otherwise the log level will be the input integer.
 */
class CV_EXPORTS_W_SIMPLE libmv_ReconstructionOptions
{
public:
  CV_WRAP
  libmv_ReconstructionOptions(const int _keyframe1=1,
                              const int _keyframe2=2,
                              const int _refine_intrinsics=1,
                              const int _select_keyframes=1,
                              const int _verbosity_level=-1)
    : keyframe1(_keyframe1), keyframe2(_keyframe2),
      refine_intrinsics(_refine_intrinsics),
      select_keyframes(_select_keyframes),
      verbosity_level(_verbosity_level) {}

  CV_PROP_RW int keyframe1, keyframe2;
  CV_PROP_RW int refine_intrinsics;
  CV_PROP_RW int select_keyframes;
  CV_PROP_RW int verbosity_level;
};

////////////////////////////////////////
// Based on 'libmv_capi' (blender API)
///////////////////////////////////////

struct libmv_Reconstruction {
  EuclideanReconstruction reconstruction;
  /* Used for per-track average error calculation after reconstruction */
  Tracks tracks;
  CameraIntrinsics *intrinsics;
  double error;
  bool is_valid;
};

libmv_Reconstruction *libmv_solveReconstructionImpl(
  const std::vector<String> &images,
  const libmv_CameraIntrinsicsOptions* libmv_camera_intrinsics_options,
  libmv_ReconstructionOptions* libmv_reconstruction_options)
{
  Ptr<Feature2D> edetector = ORB::create(10000);
  Ptr<Feature2D> edescriber = cv::xfeatures2d::DAISY::create();
  std::vector<std::string> sImages;
  for (unsigned int i=0;i<images.size();i++)
      sImages.push_back(images[i].c_str());
  libmv::correspondence::nRobustViewMatching nViewMatcher(edetector, edescriber);
  nViewMatcher.computeCrossMatch(sImages);

  // Building tracks
  libmv::Tracks tracks;
  libmv::Matches matches = nViewMatcher.getMatches();
  std::set<Matches::ImageID>::const_iterator iter_image = matches.get_images().begin();

  bool is_first_time = true;
  for (; iter_image != matches.get_images().end(); ++iter_image) {
    // Exports points
    Matches::Features<PointFeature> pfeatures = matches.InImage<PointFeature>(*iter_image);

    while(pfeatures) {
      double x = pfeatures.feature()->x(),
             y = pfeatures.feature()->y();

      // valid marker
      if ( x > 0 && y > 0 )
      {
        tracks.Insert(*iter_image, pfeatures.track(), x, y);
        if ( is_first_time )
          is_first_time = false;
      }

      // lost track
      else if ( x < 0 && y < 0 )
        is_first_time = true;

      pfeatures.operator++();
    }
  }

  // Perform reconstruction
  libmv_Reconstruction *libmv_reconstruction = new libmv_Reconstruction();
  EuclideanReconstruction &reconstruction = libmv_reconstruction->reconstruction;

  /* Retrieve reconstruction options from C-API to libmv API. */
  CameraIntrinsics *camera_intrinsics = NULL;
  switch (libmv_camera_intrinsics_options->distortion_model) {
    case libmv::DISTORTION_MODEL_POLYNOMIAL:
      camera_intrinsics = new PolynomialCameraIntrinsics();
      break;
    case libmv::DISTORTION_MODEL_DIVISION:
      camera_intrinsics = new DivisionCameraIntrinsics();
      break;
    default:
      assert(!"Unknown distortion model");
  }
  camera_intrinsics->SetFocalLength(libmv_camera_intrinsics_options->focal_length_x, libmv_camera_intrinsics_options->focal_length_y);
  camera_intrinsics->SetPrincipalPoint(libmv_camera_intrinsics_options->principal_point_x, libmv_camera_intrinsics_options->principal_point_y);
  camera_intrinsics->SetImageSize(libmv_camera_intrinsics_options->image_width, libmv_camera_intrinsics_options->image_height);

  switch (libmv_camera_intrinsics_options->distortion_model) {
    case libmv::DISTORTION_MODEL_POLYNOMIAL:
      {
        PolynomialCameraIntrinsics *polynomial_intrinsics = static_cast<PolynomialCameraIntrinsics*>(camera_intrinsics);
        polynomial_intrinsics->SetRadialDistortion(libmv_camera_intrinsics_options->polynomial_k1, libmv_camera_intrinsics_options->polynomial_k2, libmv_camera_intrinsics_options->polynomial_k3);
        break;
      }

    case libmv::DISTORTION_MODEL_DIVISION:
      {
        DivisionCameraIntrinsics *division_intrinsics = static_cast<DivisionCameraIntrinsics*>(camera_intrinsics);
        division_intrinsics->SetDistortion(libmv_camera_intrinsics_options->division_k1, libmv_camera_intrinsics_options->division_k2);
        break;
      }

    default:
      assert(!"Unknown distortion model");
  }
  libmv_reconstruction->intrinsics = camera_intrinsics;

  /* Invert the camera intrinsics/ */
  Tracks normalized_tracks;
  libmv::vector<libmv::Marker> markers = tracks.AllMarkers();
  for (int i = 0; i < markers.size(); ++i) {
    libmv::Marker &marker = markers[i];
    camera_intrinsics->InvertIntrinsics(marker.x, marker.y, &marker.x, &marker.y);
    normalized_tracks.Insert(marker.image, marker.track, marker.x, marker.y, marker.weight);
  }

  /* keyframe selection. */
  int keyframe1 = libmv_reconstruction_options->keyframe1,
      keyframe2 = libmv_reconstruction_options->keyframe2;

  if (libmv_reconstruction_options->select_keyframes) {

    libmv::vector<int> keyframes;

    /* Get list of all keyframe candidates first. */
    SelectKeyframesBasedOnGRICAndVariance(normalized_tracks, *camera_intrinsics, keyframes);

    if (keyframes.size() < 2) {
      //invalid
    } else if (keyframes.size() == 2) {
      keyframe1 = keyframes[0];
      keyframe2 = keyframes[1];
    } else {

      int previous_keyframe = keyframes[0];
      double best_error = std::numeric_limits<double>::max();
      for (int i = 1; i < keyframes.size(); i++) {
        EuclideanReconstruction reconstruction;
        int current_keyframe = keyframes[i];
        libmv::vector<Marker> keyframe_markers = normalized_tracks.MarkersForTracksInBothImages(previous_keyframe, current_keyframe);

        Tracks keyframe_tracks(keyframe_markers);

        /* get a solution from two keyframes only */
        EuclideanReconstructTwoFrames(keyframe_markers, &reconstruction);
        EuclideanBundle(keyframe_tracks, &reconstruction);
        EuclideanCompleteReconstruction(keyframe_tracks, &reconstruction, NULL);

        double current_error = EuclideanReprojectionError(tracks, reconstruction, *camera_intrinsics);
        if (current_error < best_error) {
          best_error = current_error;
          keyframe1 = previous_keyframe;
          keyframe2 = current_keyframe;
        }
        previous_keyframe = current_keyframe;
      }
    }

    /* so keyframes in the interface would be updated */
    libmv_reconstruction_options->keyframe1 = keyframe1;
    libmv_reconstruction_options->keyframe2 = keyframe2;
  }

  /* Actual reconstruction. */
  libmv::vector<Marker> keyframe_markers = normalized_tracks.MarkersForTracksInBothImages(keyframe1, keyframe2);
  if (keyframe_markers.size() < 8) {
    libmv_reconstruction->is_valid = false;
    return libmv_reconstruction;
  }

  EuclideanReconstructTwoFrames(keyframe_markers, &reconstruction);
  EuclideanBundle(normalized_tracks, &reconstruction);
  EuclideanCompleteReconstruction(normalized_tracks, &reconstruction, NULL);

  /* Refinement/ */
  if (libmv_reconstruction_options->refine_intrinsics) {
    /* only a few combinations are supported but trust the caller/ */
    int bundle_intrinsics = 0;
    if (libmv_reconstruction_options->refine_intrinsics & libmv::BUNDLE_FOCAL_LENGTH)
      bundle_intrinsics |= libmv::BUNDLE_FOCAL_LENGTH;
    if (libmv_reconstruction_options->refine_intrinsics & libmv::BUNDLE_PRINCIPAL_POINT)
      bundle_intrinsics |= libmv::BUNDLE_PRINCIPAL_POINT;
    if (libmv_reconstruction_options->refine_intrinsics & libmv::BUNDLE_RADIAL_K1)
      bundle_intrinsics |= libmv::BUNDLE_RADIAL_K1;
    if (libmv_reconstruction_options->refine_intrinsics & libmv::BUNDLE_RADIAL_K2)
      bundle_intrinsics |= libmv::BUNDLE_RADIAL_K2;
    EuclideanBundleCommonIntrinsics(tracks, bundle_intrinsics, libmv::BUNDLE_NO_CONSTRAINTS, &reconstruction, camera_intrinsics);
  }

  /* Set reconstruction scale to unity. */
  EuclideanScaleToUnity(&reconstruction);

  EuclideanReconstruction &euclidean = libmv_reconstruction->reconstruction;
  libmv_reconstruction->tracks = tracks;
  libmv_reconstruction->error = EuclideanReprojectionError(tracks, euclidean, *camera_intrinsics);
  libmv_reconstruction->is_valid = true;
  return (libmv_Reconstruction *) libmv_reconstruction;
}

#endif
