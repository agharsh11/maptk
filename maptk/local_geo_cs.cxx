/*ckwg +29
 * Copyright 2013-2017 by Kitware, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of Kitware, Inc. nor the names of any contributors may be used
 *    to endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * \file
 * \brief local_geo_cs implementation
 */

#include "local_geo_cs.h"

#include <fstream>
#include <iomanip>

#include <vital/vital_foreach.h>
#include <vital/video_metadata/video_metadata_traits.h>

#define _USE_MATH_DEFINES
#include <math.h>

#if defined M_PIl
#define LOCAL_PI M_PIl
#else
#define LOCAL_PI M_PI
#endif


using namespace kwiver::vital;

namespace kwiver {
namespace maptk {


/// scale factor converting radians to degrees
  static const double rad2deg = static_cast<double>( 180.0 ) / LOCAL_PI;
/// scale factor converting degrees to radians
  static const double deg2rad = static_cast<double>( LOCAL_PI ) / 180.0;


/// Constructor
local_geo_cs
::local_geo_cs(algo::geo_map_sptr alg)
: geo_map_algo_(alg),
  utm_origin_(0.0, 0.0, 0.0),
  utm_origin_zone_(-1)
{
}


/// Use the pose data provided by metadata to update camera pose
void
local_geo_cs
::update_camera(vital::video_metadata const& md,
                vital::simple_camera& cam,
                vital::rotation_d const& rot_offset) const
{
  if( !geo_map_algo_ )
  {
    return;
  }

  if( md.has( vital::VITAL_META_SENSOR_YAW_ANGLE) &&
      md.has( vital::VITAL_META_SENSOR_PITCH_ANGLE) &&
      md.has( vital::VITAL_META_SENSOR_ROLL_ANGLE) )
  {
    double yaw = md.find( vital::VITAL_META_SENSOR_YAW_ANGLE ).as_double();
    double pitch = md.find( vital::VITAL_META_SENSOR_PITCH_ANGLE ).as_double();
    double roll = md.find( vital::VITAL_META_SENSOR_ROLL_ANGLE ).as_double();

    // Apply offset rotation specifically on the lhs of the INS
    cam.set_rotation(rot_offset * rotation_d(yaw * deg2rad,
                                             pitch * deg2rad,
                                             roll * deg2rad));
  }

  if( md.has( vital::VITAL_META_SENSOR_LOCATION) &&
      md.has( vital::VITAL_META_SENSOR_ALTITUDE) )
  {
    double alt = md.find( vital::VITAL_META_SENSOR_ALTITUDE ).as_double();
    vital::geo_lat_lon gll;
    md.find( vital::VITAL_META_SENSOR_LOCATION ).data( gll );

    // TODO: Possibly add a positional offset optional parameter, also.
    double x,y;
    int zone;
    bool is_north_hemi;
    geo_map_algo_->latlon_to_utm(gll.latitude(), gll.longitude(),
                                 x, y, zone, is_north_hemi, utm_origin_zone_);
    cam.set_center(vector_3d(x, y, alt) - utm_origin_);
  }
}


/// Use the camera pose to update the metadata structure
void
local_geo_cs
::update_metadata(vital::simple_camera const& cam,
                  vital::video_metadata& md) const
{
  if( !geo_map_algo_ )
  {
    return;
  }
  double yaw, pitch, roll;
  cam.rotation().get_yaw_pitch_roll(yaw, pitch, roll);
  yaw *= rad2deg;
  pitch *= rad2deg;
  roll *= rad2deg;
  vital::vector_3d c = cam.get_center() + utm_origin_;
  double lat, lon;
  geo_map_algo_->utm_to_latlon(c.x(), c.y(), utm_origin_zone_, true,
                               lat, lon);

  vital::geo_lat_lon latlon( lat, lon );
  md.add( NEW_METADATA_ITEM( VITAL_META_SENSOR_LOCATION, latlon ) );
  md.add( NEW_METADATA_ITEM( VITAL_META_SENSOR_ALTITUDE, c.z() ) );
  md.add( NEW_METADATA_ITEM( VITAL_META_SENSOR_YAW_ANGLE, yaw ) );
  md.add( NEW_METADATA_ITEM( VITAL_META_SENSOR_PITCH_ANGLE, pitch ) );
  md.add( NEW_METADATA_ITEM( VITAL_META_SENSOR_ROLL_ANGLE, roll ) );
}


/// Read a local_geo_cs from a text file
void
read_local_geo_cs_from_file(local_geo_cs& lgcs,
                            vital::path_t const& file_path)
{
  std::ifstream ifs(file_path);
  double lat, lon, alt;
  ifs >> lat >> lon >> alt;
  double x,y;
  int zone;
  bool is_north_hemi;
  lgcs.geo_map_algo()->latlon_to_utm(lat, lon, x, y, zone, is_north_hemi);
  lgcs.set_utm_origin_zone(zone);
  lgcs.set_utm_origin(kwiver::vital::vector_3d(x, y, alt));
}


/// Write a local_geo_cs to a text file
void
write_local_geo_cs_to_file(local_geo_cs const& lgcs,
                           vital::path_t const& file_path)
{
  // write out the origin of the local coordinate system
  double easting = lgcs.utm_origin()[0];
  double northing = lgcs.utm_origin()[1];
  double altitude = lgcs.utm_origin()[2];
  int zone = lgcs.utm_origin_zone();
  double lat, lon;
  lgcs.geo_map_algo()->utm_to_latlon(easting, northing, zone, true, lat, lon);
  std::ofstream ofs(file_path);
  if (ofs)
  {
    ofs << std::setprecision(12) << lat << " " << lon << " " << altitude;
  }
}



/// Use a sequence of metadata objects to initialize a sequence of cameras
std::map<vital::frame_id_t, vital::camera_sptr>
initialize_cameras_with_metadata(std::map<vital::frame_id_t,
                                          vital::video_metadata_sptr> const& md_map,
                                 vital::simple_camera const& base_camera,
                                 local_geo_cs& lgcs,
                                 vital::rotation_d const& rot_offset)
{
  std::map<frame_id_t, camera_sptr> cam_map;
  vital::vector_3d mean(0,0,0);
  simple_camera active_cam(base_camera);

  bool update_local_origin = false;
  if( lgcs.utm_origin_zone() < 0 && !md_map.empty())
  {
    // if a local coordinate system has not been established,
    // use the coordinates of the first camera
    vital::video_metadata_sptr md = nullptr;
    for( auto m : md_map )
    {
      if( m.second )
      {
        md = m.second;
        break;
      }
    }
    if( md &&
        md->has( vital::VITAL_META_SENSOR_LOCATION ) )
    {
      vital::geo_lat_lon gll;
      md->find( vital::VITAL_META_SENSOR_LOCATION ).data( gll );
      double x,y;
      int zone;
      bool is_north_hemi;
      lgcs.geo_map_algo()->latlon_to_utm(gll.latitude(), gll.longitude(),
                                         x, y, zone, is_north_hemi);
      lgcs.set_utm_origin_zone(zone);
      lgcs.set_utm_origin(vital::vector_3d(x, y, 0.0));
      update_local_origin = true;
    }
  }
  VITAL_FOREACH(auto const& p, md_map)
  {
    auto md = p.second;
    if( !md )
    {
      continue;
    }
    lgcs.update_camera(*md, active_cam, rot_offset);
    mean += active_cam.center();
    cam_map[p.first] = camera_sptr(new simple_camera(active_cam));
  }

  if( update_local_origin )
  {
    mean /= static_cast<double>(cam_map.size());
    // only use the mean easting and northing
    mean[2] = 0.0;

    // shift the UTM origin to the mean of the cameras easting and northing
    lgcs.set_utm_origin(lgcs.utm_origin() + mean);

    // shift all cameras to the new coordinate system.
    typedef std::map<frame_id_t, camera_sptr>::value_type cam_map_val_t;
    VITAL_FOREACH(cam_map_val_t const &p, cam_map)
    {
      simple_camera* cam = dynamic_cast<simple_camera*>(p.second.get());
      cam->set_center(cam->get_center() - mean);
    }
  }

  return cam_map;
}


/// Update a sequence of metadata from a sequence of cameras and local_geo_cs
void
update_metadata_from_cameras(std::map<frame_id_t, camera_sptr> const& cam_map,
                             local_geo_cs const& lgcs,
                             std::map<frame_id_t, vital::video_metadata_sptr>& md_map)
{
  if( lgcs.utm_origin_zone() < 0 )
  {
    // TODO throw an exception here?
    vital::logger_handle_t
      logger( vital::get_logger( "update_metadata_from_cameras" ) );
    LOG_WARN( logger, "local geo coordinates do not have an origin");
    return;
  }

  typedef std::map<frame_id_t, camera_sptr>::value_type cam_map_val_t;
  VITAL_FOREACH(cam_map_val_t const &p, cam_map)
  {
    auto active_md = md_map[p.first];
    if( !active_md )
    {
      md_map[p.first] = active_md = std::make_shared<vital::video_metadata>();
    }
    auto cam = dynamic_cast<vital::simple_camera*>(p.second.get());
    if( active_md && cam )
    {
      lgcs.update_metadata(*cam, *active_md);
    }
  }
}


} // end namespace maptk
} // end namespace kwiver
