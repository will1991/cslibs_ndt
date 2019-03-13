#ifndef CSLIBS_NDT_2D_CONVERSION_GRIDMAP_HPP
#define CSLIBS_NDT_2D_CONVERSION_GRIDMAP_HPP

#include <cslibs_ndt/conversion/map.hpp>
#include <cslibs_ndt_2d/dynamic_maps/gridmap.hpp>
#include <cslibs_ndt_2d/static_maps/gridmap.hpp>

namespace cslibs_ndt_2d {
namespace conversion {

template <typename T>
inline typename cslibs_ndt_2d::dynamic_maps::Gridmap<T>::Ptr from(
        const typename cslibs_ndt_2d::static_maps::Gridmap<T>::Ptr& src)
{
    using converter_t = cslibs_ndt::conversion::convert
    <cslibs_ndt::map::tags::dynamic_map,cslibs_ndt::map::tags::static_map,2,cslibs_ndt::Distribution,T>;

    return converter_t::from(src);
}

template <typename T>
inline typename cslibs_ndt_2d::static_maps::Gridmap<T>::Ptr from(
        const typename cslibs_ndt_2d::dynamic_maps::Gridmap<T>::Ptr& src)
{
    using converter_t = cslibs_ndt::conversion::convert
    <cslibs_ndt::map::tags::static_map,cslibs_ndt::map::tags::dynamic_map,2,cslibs_ndt::Distribution,T>;

    return converter_t::from(src);
}

}
}

#endif // CSLIBS_NDT_2D_CONVERSION_GRIDMAP_HPP
