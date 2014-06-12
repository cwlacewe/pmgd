#include <stdio.h>
#include <string>
#include <assert.h>
#include <fstream>
#include <jsoncpp/json/json.h>
#include "jarvis.h"
#include "util.h"

#undef Exception

static const char ID_STR[] = "jarvis.loader.id";

using namespace Jarvis;

static Node *get_node(Graph &db, long long id, Jarvis::StringID tag,
                        std::function<void(Node &)> node_func)
{
    NodeIterator nodes = db.get_nodes(0,
            PropertyPredicate(ID_STR, PropertyPredicate::eq, id));
    if (nodes) return &*nodes;

    Node &node = db.add_node(tag);
    node.set_property(ID_STR, id);
    if (node_func)
        node_func(node);
    return &node;
}

static Edge *get_edge(Graph &db, long long id,
        long long src_id, long long dst_id,
        Jarvis::StringID tag,
        std::function<void(Node &)> node_func,
        std::function<void(Edge &)> edge_func)
{
    EdgeIterator edges = db.get_edges();
    EdgeIterator matching_edges = edges.filter([id](const EdgeRef &e)
            { return e.get_property(ID_STR).int_value() == id
                ? pass : dont_pass; });

    if (matching_edges) return &(Edge &)*matching_edges;

    Node *src = get_node(db, src_id, 0, node_func);
    Node *dst = get_node(db, dst_id, 0, node_func);

    Edge &edge = db.add_edge(*src, *dst, tag);
    edge.set_property(ID_STR, id);
    if (edge_func)
        edge_func(edge);
    return &edge;
}

/* GraphSON Parsing */
static int get_int_value(Json::Value &obj, const char *key_str, bool remove)
{
    Json::Value key_obj = obj[key_str];
    assert(key_obj.type() == Json::intValue);
    int val = key_obj.asInt();
    if (remove)
        obj.removeMember(key_str);
    return val;
}

static std::string
get_string_value(Json::Value &obj, const char *key_str, bool remove)
{
    Json::Value key_obj = obj[key_str];
    assert(key_obj.type() == Json::stringValue);
    std::string val = key_obj.asString();
    if (remove)
        obj.removeMember(key_str);
    return val;
}

template <typename T>
static void set_property(T *elem, const char *pkey, Json::Value &pval)
{
    switch(pval.type())
    {
        case Json::nullValue:
            elem->set_property(pkey, Property());
            break;
        case Json::intValue:
            elem->set_property(pkey, pval.asInt());
            break;
        case Json::uintValue:
            elem->set_property(pkey, (long long)pval.asUInt());
            break;
        case Json::realValue:
            elem->set_property(pkey, pval.asDouble());
            break;
        case Json::stringValue:
            elem->set_property(pkey, pval.asString());
            break;
        case Json::booleanValue:
            elem->set_property(pkey, pval.asBool());
            break;
        default:
           break;
    }
}

template <typename T>
static void set_properties(T *elem, Json::Value &jnode)
{
    Json::Value::Members members(jnode.getMemberNames());
    for(Json::Value::Members::iterator it = members.begin(); 
        it != members.end(); 
        ++it )
    {
        const std::string &pkey = *it;
        set_property(elem, pkey.c_str(), jnode[pkey]);
    }
} 

static void load_nodes(Graph &db,
        Json::Value &jnodes,
        std::function<void(Node &)> node_func,
        std::function<void(Edge &)> edge_func)
{
    for (unsigned int i=0; i < jnodes.size(); i++) {
        Json::Value jnode = jnodes[i];

        jnode.removeMember("_type");
        int id = get_int_value(jnode, "_id", true);


        Transaction tx(db);
        Node *node = get_node(db, id, 0, node_func);
        set_properties(node, jnode);
        tx.commit();
    }
}

static int load_edges(Graph &db,
        Json::Value &jedges,
        std::function<void(Node &)> node_func,
        std::function<void(Edge &)> edge_func)
{
    for (unsigned int i=0; i < jedges.size(); i++) {
        Json::Value jedge = jedges[i];

        //std::string _type = get_string_value(jedge, "_type", true);
        jedge.removeMember("_type");
        int id = get_int_value(jedge, "_id", true);
        int inv = get_int_value(jedge, "_inV", true);
        int outv = get_int_value(jedge, "_outV", true);
        std::string label = get_string_value(jedge, "_label", true);

        Transaction tx(db);
        Edge *edge = get_edge(
                db, id, outv, inv, label.c_str(), node_func, edge_func);
        set_properties(edge, jedge);
        tx.commit();
    }

    return 0;
}

static void load_gson(Graph &db,
                Json::Value &root,
                std::function<void(Node &)> node_func,
                std::function<void(Edge &)> edge_func)
{
    Json::Value jnodes = root["vertices"];

    if (jnodes.type() != Json::arrayValue) {
        throw Jarvis::Exception(203, "nodes_not_found", __FILE__, __LINE__);
    }
    load_nodes(db, jnodes, node_func, edge_func);

    Json::Value jedges = root["edges"];
    if (jedges.type() != Json::arrayValue) {
        throw Jarvis::Exception(203, "edges_not_found", __FILE__, __LINE__);
    }
    load_edges(db, jedges, node_func, edge_func);
}

void load_gson(Graph &db, const char *filename,
                std::function<void(Node &)> node_func,
                std::function<void(Edge &)> edge_func)
{
    std::ifstream input(filename);
    if (!input.is_open())
        throw Jarvis::Exception(201, "load_failed", __FILE__, __LINE__);

    Json::Value root;
    const Json::Features features;
    Json::Reader reader(features);

    if (!reader.parse(input, root)) {
        input.close();
        throw Jarvis::Exception(202, "parse_failed", __FILE__, __LINE__);
    }
    input.close();

    load_gson(db, root, node_func, edge_func);
}