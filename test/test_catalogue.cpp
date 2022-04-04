#include <gtest/gtest.h>
#include <transport_catalogue.h>

using namespace std;
using namespace tc;

TEST(Catalogue, AddSearchStop) {
    TransportCatalogue tc;

    domain::Stop stop = {"mjPsgkOt fL4kHcQl"s, {38.656967, 34.890373}};
    tc.AddStop(stop);

    auto s = tc.SearchStop(stop.name);

    ASSERT_EQ(stop.name, s->name);
    ASSERT_EQ(stop.coordinates, s->coordinates);
}

TEST(Catalogue, AddSharedPtrStop) {
    TransportCatalogue tc;

    shared_ptr<domain::Stop> stop(new domain::Stop({"adtrgxfg13"s, {8.654367, 64.892223}}));
    tc.AddStop(stop);

    auto s = tc.SearchStop(stop->name);

    ASSERT_EQ(stop.get(), s.get());
}

TEST(Catalogue, StopNotFound) {
    TransportCatalogue tc;
    ASSERT_FALSE(tc.SearchStop("lslksf"sv).get());
}

TEST(Catalogue, OneDistanceBetweenStops) {
    TransportCatalogue tc;

    domain::Stop stop1 = {"A"s, {38.656967, 34.890373}};
    domain::Stop stop2 = {"B"s, {38.646469, 34.657259}};

    tc.AddStop(stop1);
    tc.AddStop(stop2);

    double dist = 123456;
    tc.SetDistanceBetweenStops(tc.SearchStop(stop1.name), tc.SearchStop(stop2.name), dist);

    ASSERT_EQ(dist, tc.GetDistanceBetweenStops(tc.SearchStop(stop1.name),
                                               tc.SearchStop(stop2.name)));

    ASSERT_EQ(dist, tc.GetDistanceBetweenStops(tc.SearchStop(stop2.name),
                                               tc.SearchStop(stop1.name)));
}

TEST(Catalogue, DifferentDistanceBetweenStops) {
    TransportCatalogue tc;

    domain::Stop stop1 = {"A"s, {38.656967, 34.890373}};
    domain::Stop stop2 = {"B"s, {38.646469, 34.657259}};

    tc.AddStop(stop1);
    tc.AddStop(stop2);

    double dist1 = 123.456;
    double dist2 = 654.321;
    tc.SetDistanceBetweenStops(tc.SearchStop(stop1.name), tc.SearchStop(stop2.name), dist1);
    tc.SetDistanceBetweenStops(tc.SearchStop(stop2.name), tc.SearchStop(stop1.name), dist2);

    ASSERT_EQ(dist1, tc.GetDistanceBetweenStops(tc.SearchStop(stop1.name),
                                                tc.SearchStop(stop2.name)));

    ASSERT_EQ(dist2, tc.GetDistanceBetweenStops(tc.SearchStop(stop2.name),
                                                tc.SearchStop(stop1.name)));
}

TEST(Catalogue, NotSetDistanceBetweenStops) {
    TransportCatalogue tc;

    domain::Stop stop1 = {"A"s, {38.656967, 34.890373}};
    domain::Stop stop2 = {"B"s, {38.646469, 34.657259}};

    tc.AddStop(stop1);
    tc.AddStop(stop2);

    ASSERT_EQ(
        0.0, tc.GetDistanceBetweenStops(tc.SearchStop(stop1.name), tc.SearchStop(stop2.name)));

    ASSERT_EQ(
        0.0, tc.GetDistanceBetweenStops(tc.SearchStop(stop2.name), tc.SearchStop(stop1.name)));
}