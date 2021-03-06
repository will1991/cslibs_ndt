#ifndef CSLIBS_NDT_MAP_GRIDMAP_HPP
#define CSLIBS_NDT_MAP_GRIDMAP_HPP

#include <cslibs_ndt/map/generic_map.hpp>
#include <cslibs_ndt/common/distribution.hpp>

namespace cslibs_ndt {
namespace map {
template <tags::option option_t,
          std::size_t Dim,
          typename T,
          template <typename, typename, typename...> class backend_t,
          template <typename, typename, typename...> class dynamic_backend_t>
class EIGEN_ALIGN16 Map<option_t,Dim,Distribution,T,backend_t,dynamic_backend_t> :
        public GenericMap<option_t,Dim,Distribution,T,backend_t,dynamic_backend_t>
{
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    using allocator_t = Eigen::aligned_allocator<Map<option_t,Dim,Distribution,T,backend_t,dynamic_backend_t>>;

    using ConstPtr = std::shared_ptr<const Map<option_t,Dim,Distribution,T,backend_t,dynamic_backend_t>>;
    using Ptr      = std::shared_ptr<Map<option_t,Dim,Distribution,T,backend_t,dynamic_backend_t>>;

    using base_t = GenericMap<option_t,Dim,Distribution,T,backend_t,dynamic_backend_t>;
    using typename base_t::pose_t;
    using typename base_t::transform_t;
    using typename base_t::point_t;
    using typename base_t::pointcloud_t;
    using typename base_t::index_t;
    using typename base_t::index_list_t;
    using typename base_t::distribution_t;
    using typename base_t::distribution_storage_t;
    using typename base_t::distribution_storage_ptr_t;
    using typename base_t::distribution_storage_array_t;
    using typename base_t::distribution_bundle_t;
    using typename base_t::distribution_const_bundle_t;
    using typename base_t::distribution_bundle_storage_t;
    using typename base_t::distribution_bundle_storage_ptr_t;
    using typename base_t::dynamic_distribution_storage_t;

    using base_t::GenericMap;
    inline Map(const base_t &other) : base_t(other) { }
    inline Map(base_t &&other) : base_t(other) { }

    inline void insert(const point_t &p)
    {
        index_t bi;
        if (!this->toBundleIndex(p, bi))
            return;

        distribution_bundle_t *bundle = this->getAllocate(bi);
        for (std::size_t i=0; i<this->bin_count; ++i)
            bundle->at(i)->data().add(p);
    }

    inline void insert(const typename pointcloud_t::ConstPtr &points,
                       const pose_t &points_origin = pose_t())
    {
        dynamic_distribution_storage_t storage;
        for (const auto &p : *points) {
            const point_t pm = points_origin * p;
            if (pm.isNormal()) {
                const index_t &bi = this->toBundleIndex(pm);
                distribution_t *d = storage.get(bi);
                (d ? d : &storage.insert(bi, distribution_t()))->data().add(pm);
            }
        }

        storage.traverse([this](const index_t& bi, const distribution_t &d) {
            distribution_bundle_t *bundle = this->getAllocate(bi);
            const typename distribution_t::distribution_t &dist = d.data();
            for (std::size_t i=0; i<this->bin_count; ++i)
                bundle->at(i)->data() += dist;
        });
    }

    inline T sample(const point_t &p) const
    {
        return sample(p, this->toBundleIndex(p));
    }

    inline T sample(const point_t &p,
                    const index_t &bi) const
    {
        if (!this->valid(bi))
            return T();

        distribution_bundle_t *bundle  = this->bundle_storage_->get(bi);
        auto evaluate = [this, &p, &bundle]() {
            T retval = T();
            for (std::size_t i=0; i<this->bin_count; ++i)
                retval += this->div_count * bundle->at(i)->data().sample(p);
            return retval;
        };
        return bundle ? evaluate() : T();
    }

    inline T sampleNonNormalized(const point_t &p) const
    {
        return sampleNonNormalized(p, this->toBundleIndex(p));
    }

    inline T sampleNonNormalized(const point_t &p,
                                 const index_t &bi) const
    {
        if (!this->valid(bi))
            return T();

        distribution_bundle_t *bundle = this->bundle_storage_->get(bi);
        auto evaluate = [this, &p, &bundle]() {
            T retval = T();
            for (std::size_t i=0; i<this->bin_count; ++i)
                retval += this->div_count * bundle->at(i)->data().sampleNonNormalized(p);
            return retval;
        };
        return bundle ? evaluate() : T();
    }

protected:
    virtual inline bool expandDistribution(const distribution_t* d) const override
    {
        return d && d->data().getN() >= 3;
    }
};
}
}

#endif // CSLIBS_NDT_MAP_GRIDMAP_HPP
