#include <iostream>
#include <optional>
#include <set>
#include <algorithm>

#include <fmt/core.h>
#include <fmt/compile.h>
#include <fmt/ranges.h>

#include "cgimap_plugin.hpp"
#include "hooks.hpp"
#include "cgimap/types.hpp"
#include "cgimap/request.hpp"
#include "cgimap/request_helpers.hpp"
#include "cgimap/output_formatter.hpp"

#include <ogr_geometry.h>

std::vector<HookDefinitions::CallbackHandleVariant> registered_callbacks;

enum class ElementType
{
    CHANGESET,
    NODE,
    WAY,
    RELATION
};

HookAction tag_filter(ElementType element_type, const api06::TagList& tags)
{
    if ((tags.find("foo") != tags.end()) && (tags.at("foo") == "bar"))
    {
        fmt::print(FMT_COMPILE("Tag filter match: {}={}\n"), "foo", "bar");
        return HookAction::ABORT;
    }

    return HookAction::CONTINUE;
}

HookAction tag_filter(ElementType element_type, const std::vector<std::pair<std::string, std::string> >& tags)
{
    for (const auto& [key, value] : tags)
    {
        if (key == "foo" && value == "bar")
        {
            fmt::print(FMT_COMPILE("Tag filter match: {}={}\n"), "foo", "bar");
            return HookAction::ABORT;
        }
    }

    return HookAction::CONTINUE;
}

void foo_hook()
{
    fmt::print("foo hook\n");
}

HookAction request_start_hook(const request& req, const std::optional<osm_user_id_t>& user_id, const std::set<osm_user_role_t>& roles)
{
    const std::string ip = fcgi_get_env(req, "REMOTE_ADDR");
    const char *request_uri = req.get_param("REQUEST_URI");
    if (user_id.has_value())
    {
        fmt::print(FMT_COMPILE("Request start hook: user id: {}, ip: {}, path: {}\n"), user_id.value(), ip, request_uri);
    }
    else
    {
        fmt::print(FMT_COMPILE("Request start hook: user id: <unauthenticated>, ip: {}, path: {}\n"), ip, request_uri);
    }

    // static size_t counter = 0;
    // if (++counter % 2 == 0)
    // {
    //     return Hooks::HookAction::ABORT; // TODO: testing
    // }
    
    return HookAction::CONTINUE;
}

HookAction write_request_hook(const request& req, const std::string& payload, const std::string& ip, const std::optional<osm_user_id_t>& user_id)
{
    const char *request_uri = req.get_param("REQUEST_URI");
    fmt::print(FMT_COMPILE("Write request hook: user id: {}, ip: {}, path: {}, payload: \n{}\n"), user_id.value_or(0), ip, request_uri, payload);    

    return HookAction::CONTINUE;
}

HookAction changeset_create_hook(osm_user_id_t user_id, const api06::TagList& tags)
{
    fmt::print(FMT_COMPILE("Changeset create hook: user id: {}, tags: {}\n"), user_id, tags);

    auto filter_action = tag_filter(ElementType::CHANGESET, tags);
    if (filter_action != HookAction::CONTINUE)
        return filter_action;

    return HookAction::CONTINUE;
}

HookAction changeset_upload_hook(osm_user_id_t user_id, osm_changeset_id_t changeset, const api06::OSMChange_Handler& osmchange_handler, const api06::OSMChange_Tracking& change_tracking, const std::vector<api06::diffresult_t> diffresult, bbox_t upload_bbox, bbox_t changeset_bbox)
{
    fmt::print(FMT_COMPILE("Changeset upload hook: user id: {}, changeset: {}, upload bbox: {}, changeset bbox: {}\n"), user_id, changeset, upload_bbox.as_double(), changeset_bbox.as_double());

    fmt::print(FMT_COMPILE("Diffresult:\n"));
    for (const auto& d : diffresult)
    {
        auto elt_type_name = osm_object_type_name(d.obj_type);
        auto op_name = osm_operation_name(d.op);
        fmt::print(FMT_COMPILE("{}: {} {} -> {}:{}\n"), elt_type_name, op_name, d.old_id, d.new_id, d.new_version);
    }

    auto wgs84 = OGRSpatialReference::GetWGS84SRS();

    auto [minlat, minlon, maxlat, maxlon] = changeset_bbox.as_double();
    OGRLinearRing bbox_ring;
    bbox_ring.assignSpatialReference(wgs84);
    bbox_ring.addPoint(minlat, minlon);
    bbox_ring.addPoint(minlat, maxlon);
    bbox_ring.addPoint(maxlat, maxlon);
    bbox_ring.addPoint(maxlat, minlon);
    bbox_ring.closeRings();

    OGRPolygon bbox_poly;
    bbox_poly.addRing(&bbox_ring);
    bbox_poly.assignSpatialReference(wgs84);

    auto bbox_area = bbox_poly.get_Area();
    auto bbox_area_wgs84 = bbox_poly.get_GeodesicArea(wgs84); // TODO: sometimes returns nan
    // auto bbox_area_wgs84 = bbox_poly.get_GeodesicArea();
    fmt::print(FMT_COMPILE("bbox area: {}, geodesic area: {}m²\n"), bbox_area, bbox_area_wgs84);


    if (bbox_area_wgs84 >= 15000000000000)
    {
        return HookAction::ABORT;
    }

    return HookAction::CONTINUE;
}

HookAction new_node_hook(const ApiDB_Node_Updater::node_t& node)
{
    fmt::print(FMT_COMPILE("New node in changeset {}: {}/{}:{}, lat={}, lon={}, tags={}\n"), node.changeset_id, node.old_id, node.id, node.version, node.lat, node.lon, node.tags);

    auto filter_action = tag_filter(ElementType::CHANGESET, node.tags);
    if (filter_action != HookAction::CONTINUE)
        return filter_action;

    return HookAction::CONTINUE;
}

HookAction modified_node_hook(const ApiDB_Node_Updater::node_t& node)
{
    fmt::print(FMT_COMPILE("Modified node in changeset {}: {}/{}:{}, lat={}, lon={}, tags={}\n"), node.changeset_id, node.old_id, node.id, node.version, node.lat, node.lon, node.tags);

    auto filter_action = tag_filter(ElementType::CHANGESET, node.tags);
    if (filter_action != HookAction::CONTINUE)
        return filter_action;

    return HookAction::CONTINUE;
}

HookAction deleted_node_hook(const ApiDB_Node_Updater::node_t& node)
{
    fmt::print(FMT_COMPILE("Deleted node in changeset {}: {}/{}:{}, lat={}, lon={}, tags={}\n"), node.changeset_id, node.old_id, node.id, node.version, node.lat, node.lon, node.tags);

    auto filter_action = tag_filter(ElementType::CHANGESET, node.tags);
    if (filter_action != HookAction::CONTINUE)
        return filter_action;

    return HookAction::CONTINUE;
}

HookAction new_way_hook(const ApiDB_Way_Updater::way_t& way)
{
    std::vector<osm_nwr_id_t> way_nodes;
    for (const auto& n : way.way_nodes)
        way_nodes.emplace_back(n.node_id);
    fmt::print(FMT_COMPILE("New way in changeset {}: {}/{}:{}, nodes={}, tags={}\n"), way.changeset_id, way.old_id, way.id, way.version, way_nodes, way.tags);

    auto filter_action = tag_filter(ElementType::CHANGESET, way.tags);
    if (filter_action != HookAction::CONTINUE)
        return filter_action;

    return HookAction::CONTINUE;
}

HookAction modified_way_hook(const ApiDB_Way_Updater::way_t& way)
{
    std::vector<osm_nwr_id_t> way_nodes;
    for (const auto& n : way.way_nodes)
        way_nodes.emplace_back(n.node_id);
    fmt::print(FMT_COMPILE("Modified way in changeset {}: {}/{}:{}, nodes={}, tags={}\n"), way.changeset_id, way.old_id, way.id, way.version, way_nodes, way.tags);

    auto filter_action = tag_filter(ElementType::CHANGESET, way.tags);
    if (filter_action != HookAction::CONTINUE)
        return filter_action;

    return HookAction::CONTINUE;
}

HookAction deleted_way_hook(const ApiDB_Way_Updater::way_t& way)
{
    std::vector<osm_nwr_id_t> way_nodes;
    for (const auto& n : way.way_nodes)
        way_nodes.emplace_back(n.node_id);
    fmt::print(FMT_COMPILE("Deleted way in changeset {}: {}/{}:{}, nodes={}, tags={}\n"), way.changeset_id, way.old_id, way.id, way.version, way_nodes, way.tags);

    auto filter_action = tag_filter(ElementType::CHANGESET, way.tags);
    if (filter_action != HookAction::CONTINUE)
        return filter_action;

    return HookAction::CONTINUE;
}

extern "C" int init_plugin()
{
    std::cout << "Hello from plugin foo" << std::endl;
    registered_callbacks.emplace_back(Hook<HookId::POST_PLUGIN_LOAD>::register_callback(foo_hook));
    registered_callbacks.emplace_back(Hook<HookId::REQUEST_START>::register_callback(request_start_hook));
    registered_callbacks.emplace_back(Hook<HookId::WRITE_REQUEST>::register_callback(write_request_hook));
    registered_callbacks.emplace_back(Hook<HookId::CHANGESET_CREATE>::register_callback(changeset_create_hook));
    registered_callbacks.emplace_back(Hook<HookId::CHANGESET_UPLOAD>::register_callback(changeset_upload_hook));
    registered_callbacks.emplace_back(Hook<HookId::NODE_CREATED>::register_callback(new_node_hook));
    registered_callbacks.emplace_back(Hook<HookId::NODE_MODIFIED>::register_callback(modified_node_hook));
    registered_callbacks.emplace_back(Hook<HookId::NODE_DELETED>::register_callback(deleted_node_hook));
    registered_callbacks.emplace_back(Hook<HookId::WAY_CREATED>::register_callback(new_way_hook));
    registered_callbacks.emplace_back(Hook<HookId::WAY_MODIFIED>::register_callback(modified_way_hook));
    registered_callbacks.emplace_back(Hook<HookId::WAY_DELETED>::register_callback(deleted_way_hook));
    return 0;
}

extern "C" int deinit_plugin()
{
    std::cout << "Goodbye from plugin foo" << std::endl;
    return 0;
}

extern "C" const char* plugin_version()
{
    return "v0.1";
}

int foo_plugin()
{
    return 0;
}

extern "C" void foo_target_function();

__attribute__((constructor)) void foo_ctor()
{
    std::cout << "Hello from plugin foo constructor" << std::endl;
    foo_target_function();
}

__attribute__((destructor)) void foo_dtor()
{
    std::cout << "Goodbye from plugin foo destructor" << std::endl;
}
