#include <gtest/gtest.h>
#include <transport_catalogue.h>

using namespace std;
using namespace tc;

TEST(Catalogue, AddStop) {
    TransportCatalogue tc;

    domain::Stop stop = {"mjPsgkOt fL4kHcQl"s, {38.656967, 34.890373}};
    tc.AddStop(stop);

    auto s = tc.SearchStop(stop.name);

    ASSERT_EQ(stop.name, s->name);
    ASSERT_EQ(stop.coordinates, s->coordinates);
}
