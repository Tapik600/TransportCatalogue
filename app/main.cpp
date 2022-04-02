#include <json_reader.h>
#include <serialization.h>
#include <transport_router.h>

#include <fstream>
#include <iostream>
#include <string_view>

using namespace std::literals;
using namespace tc;

void PrintUsage(std::ostream &stream = std::cerr) {
    stream << "Usage: transport_catalogue [make_base|process_requests]\n"sv;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        PrintUsage();
        return 1;
    }

    const std::string_view mode(argv[1]);

    if (mode == "make_base"sv) {

        tc::TransportCatalogue db;
        json::reader::JsonReader reader;

        reader.ReadRequests(std::cin);
        reader.ExecuteBaseRequest(db);

        renderer::MapRenderer map_renderer(reader.GetRendererSettings(), db.GetBuses());
        router::TransportRouter router(db, reader.GetRoutingSettings());
        serialize::Serializer serializer(reader.GetSerializationSettings());

        serializer.Serialize(db, map_renderer, router);
        if (!serializer.Save()) {
            return 1;
        }

    } else if (mode == "process_requests"sv) {

        json::reader::JsonReader reader;
        reader.ReadRequests(std::cin);

        serialize::Serializer serializer(reader.GetSerializationSettings());
        if (!serializer.Load()) {
            std::cout << "file not opening!"sv;
            return 1;
        }

        tc::TransportCatalogue catalogue = std::move(serializer.GetTransportCatalogue());
        router::TransportRouter router(
            catalogue, serializer.GetRouterVertexes(), serializer.GetRouterEdgesInfo(),
            serializer.GetRouterGraph(), serializer.GetRouterInternalData());
        renderer::MapRenderer map_renderer(serializer.GetRendererSettings(),
                                           catalogue.GetBuses());

        tc::RequestHandler handler(catalogue, map_renderer, router);

        reader.ExecuteStatRequest(std::cout, handler);

    } else {
        PrintUsage();
        return 1;
    }
    return 0;
}
