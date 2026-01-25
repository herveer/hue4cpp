#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <hue4cpp/color_utils.h>

using namespace hue4cpp;

TEST_CASE("RGB to XY conversion", "[color][conversion]") {
    SECTION("Pure red") {
        RGBColor red(255, 0, 0);
        XYColor xy = color_utils::rgbToXy(red);
        
        // Red should have high x value, low y value
        REQUIRE(xy.x > 0.6f);
        REQUIRE(xy.y < 0.4f);
    }
    
    SECTION("Pure green") {
        RGBColor green(0, 255, 0);
        XYColor xy = color_utils::rgbToXy(green);
        
        // Green should have moderate x value, high y value
        REQUIRE(xy.x < 0.4f);
        REQUIRE(xy.y > 0.6f);
    }
    
    SECTION("Pure blue") {
        RGBColor blue(0, 0, 255);
        XYColor xy = color_utils::rgbToXy(blue);
        
        // Blue should have low x and y values
        REQUIRE(xy.x < 0.2f);
        REQUIRE(xy.y < 0.2f);
    }
    
    SECTION("White (D65)") {
        RGBColor white(255, 255, 255);
        XYColor xy = color_utils::rgbToXy(white);
        
        // White should be in the central region
        // The exact value depends on the color space conversion matrix used
        REQUIRE(xy.x > 0.30f);
        REQUIRE(xy.x < 0.35f);
        REQUIRE(xy.y > 0.30f);
        REQUIRE(xy.y < 0.35f);
    }
    
    SECTION("Black") {
        RGBColor black(0, 0, 0);
        XYColor xy = color_utils::rgbToXy(black);
        
        // Black should map to (0, 0)
        REQUIRE(xy.x == 0.0f);
        REQUIRE(xy.y == 0.0f);
    }
}

TEST_CASE("XY to RGB conversion", "[color][conversion]") {
    SECTION("White point at full brightness") {
        XYColor white(0.3127f, 0.3290f);
        RGBColor rgb = color_utils::xyToRgb(white, 100);
        
        // Should be close to white
        REQUIRE(rgb.r > 200.0f);
        REQUIRE(rgb.g > 200.0f);
        REQUIRE(rgb.b > 200.0f);
    }
    
    SECTION("White point at half brightness") {
        XYColor white(0.3127f, 0.3290f);
        RGBColor rgb = color_utils::xyToRgb(white, 50);
        
        // RGB values should be lower due to brightness
        REQUIRE(rgb.r < 200.0f);
        REQUIRE(rgb.g < 200.0f);
        REQUIRE(rgb.b < 200.0f);
    }
}

TEST_CASE("HSV to RGB conversion", "[color][conversion]") {
    SECTION("Red (H=0)") {
        RGBColor rgb = color_utils::hsvToRgb(0, 100, 100);
        REQUIRE(rgb.r == Catch::Approx(255.0f).epsilon(1.0));
        REQUIRE(rgb.g == Catch::Approx(0.0f).epsilon(1.0));
        REQUIRE(rgb.b == Catch::Approx(0.0f).epsilon(1.0));
    }
    
    SECTION("Green (H=120)") {
        RGBColor rgb = color_utils::hsvToRgb(120, 100, 100);
        REQUIRE(rgb.r == Catch::Approx(0.0f).epsilon(1.0));
        REQUIRE(rgb.g == Catch::Approx(255.0f).epsilon(1.0));
        REQUIRE(rgb.b == Catch::Approx(0.0f).epsilon(1.0));
    }
    
    SECTION("Blue (H=240)") {
        RGBColor rgb = color_utils::hsvToRgb(240, 100, 100);
        REQUIRE(rgb.r == Catch::Approx(0.0f).epsilon(1.0));
        REQUIRE(rgb.g == Catch::Approx(0.0f).epsilon(1.0));
        REQUIRE(rgb.b == Catch::Approx(255.0f).epsilon(1.0));
    }
    
    SECTION("Gray (S=0)") {
        RGBColor rgb = color_utils::hsvToRgb(0, 0, 50);
        REQUIRE(rgb.r == Catch::Approx(127.5f).epsilon(1.0));
        REQUIRE(rgb.g == Catch::Approx(127.5f).epsilon(1.0));
        REQUIRE(rgb.b == Catch::Approx(127.5f).epsilon(1.0));
    }
}

TEST_CASE("RGB to HSV conversion", "[color][conversion]") {
    SECTION("Red") {
        float h, s, v;
        color_utils::rgbToHsv(RGBColor(255, 0, 0), h, s, v);
        REQUIRE(h == Catch::Approx(0.0f).epsilon(1.0));
        REQUIRE(s == Catch::Approx(100.0f).epsilon(1.0));
        REQUIRE(v == Catch::Approx(100.0f).epsilon(1.0));
    }
    
    SECTION("Green") {
        float h, s, v;
        color_utils::rgbToHsv(RGBColor(0, 255, 0), h, s, v);
        REQUIRE(h == Catch::Approx(120.0f).epsilon(1.0));
        REQUIRE(s == Catch::Approx(100.0f).epsilon(1.0));
        REQUIRE(v == Catch::Approx(100.0f).epsilon(1.0));
    }
    
    SECTION("Blue") {
        float h, s, v;
        color_utils::rgbToHsv(RGBColor(0, 0, 255), h, s, v);
        REQUIRE(h == Catch::Approx(240.0f).epsilon(1.0));
        REQUIRE(s == Catch::Approx(100.0f).epsilon(1.0));
        REQUIRE(v == Catch::Approx(100.0f).epsilon(1.0));
    }
    
    SECTION("Black") {
        float h, s, v;
        color_utils::rgbToHsv(RGBColor(0, 0, 0), h, s, v);
        REQUIRE(h == 0.0f);
        REQUIRE(s == 0.0f);
        REQUIRE(v == 0.0f);
    }
}

TEST_CASE("HSV-RGB round trip", "[color][conversion]") {
    SECTION("Round trip conversion preserves color") {
        RGBColor original(128, 64, 192);
        float h, s, v;
        color_utils::rgbToHsv(original, h, s, v);
        RGBColor converted = color_utils::hsvToRgb(h, s, v);
        
        REQUIRE(converted.r == Catch::Approx(original.r).epsilon(1.0));
        REQUIRE(converted.g == Catch::Approx(original.g).epsilon(1.0));
        REQUIRE(converted.b == Catch::Approx(original.b).epsilon(1.0));
    }
}

TEST_CASE("Color presets - primary colors", "[color][presets]") {
    SECTION("Red") {
        REQUIRE(colors::Red.r == 255.0f);
        REQUIRE(colors::Red.g == 0.0f);
        REQUIRE(colors::Red.b == 0.0f);
    }
    
    SECTION("Green") {
        REQUIRE(colors::Green.r == 0.0f);
        REQUIRE(colors::Green.g == 255.0f);
        REQUIRE(colors::Green.b == 0.0f);
    }
    
    SECTION("Blue") {
        REQUIRE(colors::Blue.r == 0.0f);
        REQUIRE(colors::Blue.g == 0.0f);
        REQUIRE(colors::Blue.b == 255.0f);
    }
}

TEST_CASE("Color presets - secondary colors", "[color][presets]") {
    SECTION("Yellow") {
        REQUIRE(colors::Yellow.r == 255.0f);
        REQUIRE(colors::Yellow.g == 255.0f);
        REQUIRE(colors::Yellow.b == 0.0f);
    }
    
    SECTION("Cyan") {
        REQUIRE(colors::Cyan.r == 0.0f);
        REQUIRE(colors::Cyan.g == 255.0f);
        REQUIRE(colors::Cyan.b == 255.0f);
    }
    
    SECTION("Magenta") {
        REQUIRE(colors::Magenta.r == 255.0f);
        REQUIRE(colors::Magenta.g == 0.0f);
        REQUIRE(colors::Magenta.b == 255.0f);
    }
}

TEST_CASE("Get color by name", "[color][presets]") {
    SECTION("Get red by name") {
        auto color = colors::getColorByName("red");
        REQUIRE(color.has_value());
        REQUIRE(color->r == 255.0f);
        REQUIRE(color->g == 0.0f);
        REQUIRE(color->b == 0.0f);
    }
    
    SECTION("Case insensitive") {
        auto color1 = colors::getColorByName("RED");
        auto color2 = colors::getColorByName("Red");
        auto color3 = colors::getColorByName("red");
        
        REQUIRE(color1.has_value());
        REQUIRE(color2.has_value());
        REQUIRE(color3.has_value());
        REQUIRE(color1->r == color2->r);
        REQUIRE(color2->r == color3->r);
    }
    
    SECTION("Invalid color name") {
        auto color = colors::getColorByName("notacolor");
        REQUIRE_FALSE(color.has_value());
    }
    
    SECTION("Get all color names") {
        auto names = colors::getColorNames();
        REQUIRE(names.size() > 0);
        REQUIRE_FALSE(names.empty());
    }
}

TEST_CASE("Color temperature presets", "[color][presets]") {
    SECTION("Warm white is warm") {
        // Warm colors should have more red
        REQUIRE(colors::WarmWhite.r >= colors::WarmWhite.b);
    }
    
    SECTION("Cool white is cool") {
        // Cool colors should have more blue
        REQUIRE(colors::CoolWhite.b >= colors::CoolWhite.r);
    }
}
