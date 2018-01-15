#ifndef CSLIBS_NDT_3D_SERIALIZATION_STATIC_MAPS_GRIDMAP_HPP
#define CSLIBS_NDT_3D_SERIALIZATION_STATIC_MAPS_GRIDMAP_HPP

#include <cslibs_ndt/common/serialization/indexed_distribution.hpp>
#include <cslibs_ndt/common/serialization/storage.hpp>

#include <cslibs_ndt_3d/static_maps/gridmap.hpp>
#include <cslibs_ndt_3d/dynamic_maps/gridmap.hpp>
#include <cslibs_math_3d/serialization/transform.hpp>

#include <yaml-cpp/yaml.h>

#include <cslibs_math/serialization/array.hpp>
#include <cslibs_ndt/common/serialization/filesystem.hpp>
#include <fstream>

namespace cslibs_ndt_3d {
namespace static_maps {
inline bool save(const cslibs_ndt_3d::static_maps::Gridmap::Ptr &map,
                 const std::string &path)
{
    using path_t  = boost::filesystem::path;
    using paths_t = std::array<path_t, 8>;
    using index_t = std::array<int, 3>;
    using distribution_storage_ptr_t = cslibs_ndt_3d::static_maps::Gridmap::distribution_storage_ptr_t;

    /// step one: check if the root diretory exists
    path_t path_root(path);
    if(!cslibs_ndt::common::serialization::create_directory(path_root)) {
        return false;
    }

    /// step two: check if the sub folders can be created
    paths_t paths = {{path_root / path_t("0"), path_root / path_t("1"), path_root / path_t("2"), path_root / path_t("3"),
                      path_root / path_t("4"), path_root / path_t("5"), path_root / path_t("6"), path_root / path_t("7")}};

    /// step three: we have our filesystem, now we write out the distributions file by file
    /// meta file
    path_t path_file = path_t("map.yaml");
    {
        std::ofstream out = std::ofstream((path_root / path_file).string());
        YAML::Emitter yaml(out);
        YAML::Node n;
        std::vector<index_t> indices;
        map->getBundleIndices(indices);
        n["origin"] = map->getOrigin();
        n["resolution"] = map->getResolution();
        n["size"] = map->getSize();
        n["bundles"] = indices;
        yaml << n;

    }
    for(std::size_t i = 0 ; i < 8 ; ++i) {
        if(!cslibs_ndt::common::serialization::create_directory(paths[i])) {
            return false;
        }
    }

    for (std::size_t i = 0 ; i < 8 ; ++ i) {
        const distribution_storage_ptr_t storage = map->getStorages()[i];
        cslibs_ndt_3d::dynamic_maps::Gridmap::distribution_storage_ptr_t storage_kd(
                    new cslibs_ndt_3d::dynamic_maps::Gridmap::distribution_storage_t());

        storage->traverse([&storage_kd](const index_t &index, const typename cslibs_ndt_3d::static_maps::Gridmap::distribution_t &data) {
            storage_kd->insert(index, data);
        });
        cslibs_ndt::save(storage_kd, paths[i]);
    }

    return true;
}

inline bool load(cslibs_ndt_3d::static_maps::Gridmap::Ptr &map,
                 const std::string &path)
{
    using path_t  = boost::filesystem::path;
    using paths_t = std::array<path_t, 8>;
    using index_t = std::array<int, 3>;
    using map_t   = cslibs_ndt_3d::static_maps::Gridmap;

    /// step one: check if the root diretory exists
    path_t path_root(path);
    if(!cslibs_ndt::common::serialization::check_directory(path_root)) {
        return false;
    }

    /// step two: check if the sub folders can be created
    paths_t paths = {{path_root / path_t("0"), path_root /  path_t("1"), path_root /  path_t("2"), path_root / path_t("3"),
                      path_root / path_t("4"), path_root /  path_t("5"), path_root /  path_t("6"), path_root / path_t("7")}};

    /// step three: we have our filesystem, now we can load distributions file by file
    for(std::size_t i = 0 ; i < 8 ; ++i) {
        if(!cslibs_ndt::common::serialization::check_directory(paths[i])) {
            return false;
        }
    }

    /// load meta data
    path_t path_file = path_t("map.yaml");
    {
        YAML::Node n = YAML::LoadFile((path_root / path_file).string());
        const cslibs_math_3d::Transform3d origin = n["origin"].as<cslibs_math_3d::Transform3d>();
        const double resolution = n["resolution"].as<double>();
        const std::array<std::size_t, 3> size = n["size"].as<std::array<std::size_t, 3>>();
        const std::vector<index_t> indices = n["bundles"].as<std::vector<index_t>>();

        map.reset(new map_t(origin, resolution, size));

        // allocation stuff, just 4 fun
        for (const index_t& bi : indices)
            map->getDistributionBundle(bi);
    }

    const std::array<std::size_t, 3> & bundle_size = map->getBundleSize();
    auto get_bundle_index = [&bundle_size] (const index_t & si) {
        return index_t({{std::max(0, std::min(2 * si[0], static_cast<int>(bundle_size[0] - 1))),
                         std::max(0, std::min(2 * si[1], static_cast<int>(bundle_size[1] - 1))),
                         std::max(0, std::min(2 * si[2], static_cast<int>(bundle_size[2] - 1)))}});
    };

    for (std::size_t i = 0 ; i < 8 ; ++ i) {
        cslibs_ndt_3d::dynamic_maps::Gridmap::distribution_storage_ptr_t storage_kd;
        if (!cslibs_ndt::load(storage_kd, paths[i]))
            return false;

        storage_kd->traverse([&map, &i, &get_bundle_index] (const index_t & si, const typename map_t::distribution_t & d) {
            const index_t & bi = get_bundle_index(si);
            if (const typename map_t::distribution_bundle_t* b = map->getDistributionBundle(bi))
                b->at(i)->data() = d.data();
        });
    }

    return true;
}
}
}

namespace YAML {
template <>
struct convert<cslibs_ndt_3d::static_maps::Gridmap::Ptr>
{
    using map_t = cslibs_ndt_3d::static_maps::Gridmap;
    static Node encode(const typename map_t::Ptr &rhs)
    {
        Node n;
        if (!rhs)
            return n;

        n.push_back(rhs->getOrigin());
        n.push_back(rhs->getResolution());

        const std::array<std::size_t, 3> & size = rhs->getSize();
        n.push_back(size);

        using index_t = std::array<int, 3>;
        using distribution_storage_t =
        typename cslibs_ndt_3d::dynamic_maps::Gridmap::distribution_storage_t;
        using distribution_storage_ptr_t =
        typename cslibs_ndt_3d::dynamic_maps::Gridmap::distribution_storage_ptr_t;

        auto divx = [](const index_t & bi) { return cslibs_math::common::div<int>(bi[0], 2); };
        auto divy = [](const index_t & bi) { return cslibs_math::common::div<int>(bi[1], 2); };
        auto divz = [](const index_t & bi) { return cslibs_math::common::div<int>(bi[2], 2); };
        auto modx = [](const index_t & bi) { return cslibs_math::common::mod<int>(bi[0], 2); };
        auto mody = [](const index_t & bi) { return cslibs_math::common::mod<int>(bi[1], 2); };
        auto modz = [](const index_t & bi) { return cslibs_math::common::mod<int>(bi[2], 2); };

        auto get_storage_index = [&divx, &divy, &divz, &modx, &mody, &modz] (const index_t & bi, const std::size_t i) {
            return index_t({{(i % 2 == 0) ? divx(bi) : (divx(bi) + modx(bi)),
                             ((i / 2) % 2 == 0) ? divy(bi) : (divy(bi) + mody(bi)),
                             (i < 4) ? divz(bi) : (divz(bi) + modz(bi))}});
        };

        for (std::size_t i = 0 ; i < 8 ; ++ i) {
            const distribution_storage_ptr_t storage(new distribution_storage_t());

            for (int idx = 0 ; idx < static_cast<int>(rhs->getBundleSize()[0]) ; ++ idx) {
                for (int idy = 0 ; idy < static_cast<int>(rhs->getBundleSize()[1]) ; ++ idy) {
                    for (int idz = 0 ; idz < static_cast<int>(rhs->getBundleSize()[2]) ; ++ idz) {
                        const index_t bi({idx, idy, idz});

                        if (const typename map_t::distribution_bundle_t* b = rhs->getDistributionBundle(bi)) {
                            const index_t si = get_storage_index(bi, i);
                            if (!storage->get(si) && b->at(i)->data().getN() > 0)
                                storage->insert(si, *(b->at(i)));
                        }
                    }
                }
            }

            n.push_back(storage);
        }

        return n;
    }

    static bool decode(const Node& n, typename map_t::Ptr &rhs)
    {
        if (!n.IsSequence() || n.size() != 11)
            return false;

        rhs.reset(new map_t(n[0].as<cslibs_math_3d::Transform3d>(), n[1].as<double>(), n[2].as<std::array<std::size_t, 3>>()));

        using index_t = std::array<int, 3>;
        using distribution_storage_ptr_t =
        typename cslibs_ndt_3d::dynamic_maps::Gridmap::distribution_storage_ptr_t;

        const std::array<std::size_t, 3> & bundle_size = rhs->getBundleSize();
        auto get_bundle_index = [&bundle_size] (const index_t & si) {
            return index_t({{std::max(0, std::min(2 * si[0], static_cast<int>(bundle_size[0] - 1))),
                             std::max(0, std::min(2 * si[1], static_cast<int>(bundle_size[1] - 1))),
                             std::max(0, std::min(2 * si[2], static_cast<int>(bundle_size[2] - 1)))}});
        };

        for (std::size_t i = 0 ; i < 8 ; ++ i) {
            const distribution_storage_ptr_t & storage = n[3 + i].as<distribution_storage_ptr_t>();

            storage->traverse([&rhs, &i, &get_bundle_index] (const index_t & si, const typename map_t::distribution_t & d) {
                const index_t & bi = get_bundle_index(si);
                if (const typename map_t::distribution_bundle_t* b = rhs->getDistributionBundle(bi))
                    b->at(i)->data() = d.data();
            });
        }

        return true;
    }
};
}

#endif // CSLIBS_NDT_3D_SERIALIZATION_STATIC_MAPS_GRIDMAP_HPP
