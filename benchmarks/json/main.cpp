// Copyright (C) 2020 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#define ANKERL_NANOBENCH_IMPLEMENT
#include <nanobench.h>

#include <lexy/input/file.hpp>

bool json_baseline(const lexy::buffer<lexy::utf8_encoding>& input);
bool json_lexy(const lexy::buffer<lexy::utf8_encoding>& input);
bool json_pegtl(const lexy::buffer<lexy::utf8_encoding>& input);
bool json_nlohmann(const lexy::buffer<lexy::utf8_encoding>& input);
bool json_rapid(const lexy::buffer<lexy::utf8_encoding>& input);

auto get_data(const char* file_name)
{
    auto path   = std::string(LEXY_BENCHMARK_DATA) + "/" + file_name;
    auto result = lexy::read_file<lexy::utf8_encoding>(path.c_str());
    if (!result)
        throw std::runtime_error("unable to read data file");
    return LEXY_MOV(result).value();
}

const char* output_prefix()
{
    return R"(= JSON Validation Benchmark

This benchmark measures the time it takes to *validate* JSON, i.e. to check whether it is well-formed.
Validation was chosen as opposed to parsing, as parsing speed depends on the JSON data structure as well.
Implementing an efficient JSON container is out of scope for lexy, so it would have a disadvantage over the specialized JSON libraries.

The average validation times for each input are shown in the boxplots below.
Lower values are better.

[pass]
++++
<script src="https://cdn.plot.ly/plotly-latest.min.js"></script>
++++

)";
}

const char* output_template()
{
    return R"(
[pass]
++++
<div id="{{title}}"></div>
<script>
    var data = [
        {{#result}}{
            name: '{{name}}',
            y: [{{#measurement}}{{elapsed}}{{^-last}}, {{/last}}{{/measurement}}],
        },
        {{/result}}
    ];
    var title = '{{title}}';

    data = data.map(a => Object.assign(a, { boxpoints: 'all', pointpos: 0, type: 'box' }));
    var layout = { title: { text: title }, showlegend: false, yaxis: { title: 'validation time', rangemode: 'tozero', autorange: true } };
    Plotly.newPlot('{{title}}', data, layout, {responsive: true});
</script>
++++
    )";
}

const char* output_suffix()
{
    return R"(
.The implementations
`baseline`::
    This simply adds all input characters of the JSON document without performing actual validation.
`lexy`::
    A JSON validator using the lexy grammar from the example.
`pegtl`::
    A JSON validator using the https://github.com/taocpp/PEGTL[PEGTL] JSON grammar.
`nlohmann/json`::
    A JSON validator using https://github.com/nlohmann/json[JSON for Modern C++] implemented by `nlohmann::json::accept()`.
`rapidjson`::
    A JSON validator using https://github.com/Tencent/rapidjson[rapidjson] implemented using a SAX parser with the `rapidjson::BaseReaderHandler`.

.The inputs
`canada.json`::
    Contains lots of 2-element arrays holding floating-point coordinate pairs.
    Taken from https://github.com/miloyip/nativejson-benchmark.
`citm_catalog.json`::
    Big JSON file with some variety.
    Taken from https://github.com/miloyip/nativejson-benchmark.
`twitter.json`::
    Some data from twitter's API.
    Taken from https://github.com/miloyip/nativejson-benchmark.

.The Methodology
The input data is read using `lexy::read_file()`.
The resulting buffer is then passed to the various implementations using their memory inputs.
Benchmarking is done by https://nanobench.ankerl.com/[nanobench] on a https://docs.github.com/en/free-pro-team@latest/actions/reference/specifications-for-github-hosted-runners#supported-runners-and-hardware-resources[GitHub Actions virtual machine].
    )";
}

int main()
{
    std::ofstream            out("benchmark_json.adoc");
    ankerl::nanobench::Bench b;

    auto bench_data = [&](const char* file) {
        auto data = get_data(file);

        b.title(file).relative(true);
        b.unit("byte").batch(data.size());
        b.minEpochIterations(10);

        b.run("baseline", [&] { return json_baseline(data); });
        b.run("lexy", [&] { return json_lexy(data); });
        b.run("pegtl", [&] { return json_pegtl(data); });
        b.run("nlohmann/json", [&] { return json_nlohmann(data); });
        b.run("rapidjson", [&] { return json_rapid(data); });

        b.render(output_template(), out);
    };

    out << output_prefix();
    bench_data("canada.json");
    bench_data("citm_catalog.json");
    bench_data("twitter.json");
    out << output_suffix();
}

