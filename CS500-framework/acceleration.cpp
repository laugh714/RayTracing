#include "geom.h"
#include "acceleration.h"
#include "Shapes.h"

#include <bvh/sweep_sah_builder.hpp>
#include <bvh/single_ray_traverser.hpp>
#include <bvh/primitive_intersectors.hpp>

/////////////////////////////
// Vector and ray conversions
Ray RayFromBvh(const bvh::Ray<float> &r)
{
    return Ray(vec3FromBvh(r.origin), vec3FromBvh(r.direction));
}
bvh::Ray<float> RayToBvh(const Ray &r)
{
    return bvh::Ray<float>(vec3ToBvh(r.Q), vec3ToBvh(r.D));
}


/////////////////////////////
// SimpleBox
bvh::Vector3<float> vec3ToBvh(const vec3& v)
{
    return bvh::Vector3<float>(v[0],v[1],v[2]);
}

vec3 vec3FromBvh(const bvh::Vector3<float>& v)
{
    return vec3(v[0],v[1],v[2]);
}

SimpleBox::SimpleBox(): bvh::BoundingBox<float>() {}
SimpleBox::SimpleBox(const vec3 v): bvh::BoundingBox<float>(vec3ToBvh(v)) {}

SimpleBox& SimpleBox::extend(const vec3 v)
{
    bvh::BoundingBox<float>::extend(vec3ToBvh(v));
    return *this;
}


/////////////////////////////
// BvhShape

SimpleBox BvhShape::bounding_box() const
{
    //  Return the shape's bounding box.
    return shape->boundingBox;
}

bvh::Vector3<float> BvhShape::center() const
{
    return shape->boundingBox.center();
}
    
std::optional<Intersection> BvhShape::intersect(const bvh::Ray<float>& bvhray) const
{
    Intersection ret;
    if (!shape->Intersect(RayFromBvh(bvhray), ret))
        return std::nullopt;
    else if (ret.t < bvhray.tmin || ret.t > bvhray.tmax)
        return std::nullopt;
    else
        return ret;        
}

AccelerationBvh::AccelerationBvh(std::vector<Shape*> &objs)
{
    // Wrap all Shape*'s with a bvh specific instance
    for (Shape* shape:  objs) {
        shapeVector.emplace_back(shape); }

    // Magic found in the bvh examples:
    auto [bboxes, centers] = bvh::compute_bounding_boxes_and_centers(shapeVector.data(),
                                                                     shapeVector.size());
    auto global_bbox = bvh::compute_bounding_boxes_union(bboxes.get(), shapeVector.size());

    bvh::SweepSahBuilder<bvh::Bvh<float>> builder(bvh);
    builder.build(global_bbox, bboxes.get(), centers.get(), shapeVector.size());
}

Intersection AccelerationBvh::intersect(const Ray& ray)
{
    bvh::Ray<float> bvhRay = RayToBvh(ray);

    // Magic found in the bvh examples:
    bvh::ClosestPrimitiveIntersector<bvh::Bvh<float>, BvhShape> intersector(bvh, shapeVector.data());
    bvh::SingleRayTraverser<bvh::Bvh<float>> traverser(bvh);

    auto hit = traverser.traverse(bvhRay, intersector);
    if (hit) {
        return hit->intersection;
    }
    else {       
        return  Intersection();  // Return an IntersectionRecord which indicates NO-INTERSECTION
    }
}