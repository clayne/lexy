#include <lexy/callback.hpp>
#include <lexy/dsl.hpp>
#include <lexy/parse.hpp>
#include <lexy_ext/cfile.hpp>
#include <lexy_ext/report_error.hpp>

namespace dsl = lexy::dsl;

//{
struct boolean
{
    struct true_
    {
        static constexpr auto rule = LEXY_LIT("true");
        // Produce the value `true`.
        static constexpr auto value = lexy::constant(true);
    };
    struct false_
    {
        static constexpr auto rule = LEXY_LIT("false");
        // Produce the value `false`.
        static constexpr auto value = lexy::constant(false);
    };

    static constexpr auto rule = dsl::p<true_> | dsl::p<false_>;
    // Both rules produce a boolean value, just forward that one.
    static constexpr auto value = lexy::forward<bool>;
};
//}

int main()
{
    auto input = lexy_ext::read_file<>(stdin);

    auto result = lexy::parse<boolean>(input, lexy_ext::report_error);
    if (!result)
        return 1;

    std::printf("The value is: %d\n", result.value());
}

