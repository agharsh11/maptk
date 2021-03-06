# Tools

include_directories("${MAPTK_SOURCE_DIR}")
include_directories("${MAPTK_BINARY_DIR}")

get_property(maptk_libs GLOBAL PROPERTY maptk_libraries)

kwiver_add_executable(maptk_detect_and_describe detect_and_describe.cxx)
target_link_libraries(maptk_detect_and_describe
  PRIVATE             maptk vital_algo vital_vpm kwiversys
    )

kwiver_add_executable(maptk_track_features track_features.cxx)
target_link_libraries(maptk_track_features
  PRIVATE             maptk vital_algo vital_vpm kwiversys
    )

kwiver_add_executable(maptk_match_matrix match_matrix.cxx)
target_link_libraries(maptk_match_matrix
  PRIVATE             maptk kwiver_algo_core kwiversys
  )

kwiver_add_executable(maptk_pos2krtd pos2krtd.cxx)
target_link_libraries(maptk_pos2krtd
  PRIVATE             maptk vital_algo vital_vpm
  )

kwiver_add_executable(maptk_bundle_adjust_tracks bundle_adjust_tracks.cxx)
target_link_libraries(maptk_bundle_adjust_tracks
  PRIVATE             maptk kwiver_algo_core vital_vpm kwiversys
  )

kwiver_add_executable(maptk_apply_gcp apply_gcp.cxx)
target_link_libraries(maptk_apply_gcp
  PRIVATE             maptk kwiver_algo_core vital_vpm kwiversys
  )

kwiver_add_executable(maptk_analyze_tracks analyze_tracks.cxx)
target_link_libraries(maptk_analyze_tracks
  PRIVATE             maptk kwiver_algo_core vital_vpm kwiversys
  )

kwiver_add_executable(maptk_estimate_homography estimate_homography.cxx)
target_link_libraries(maptk_estimate_homography
  PRIVATE             maptk vital_algo vital_vpm kwiversys
  )
