#line 1 "playground.cpp"
#include <cctype>

int main()
{
    auto input = lexy_ext::read_file<lexy::utf8_encoding>(stdin);

    lexy::parse_tree_for<decltype(input)> tree;
    auto                                  result
        = lexy::parse_as_tree<LEXY_PLAYGROUND_PRODUCTION>(tree, input, lexy_ext::report_error);

    std::puts("graph \"Parse Tree\" {");
    for (auto [event, node] : tree.traverse())
    {
        switch (event)
        {
        case lexy::traverse_event::enter:
            std::printf("\"node-%p\" [label=\"%s\", shape=ellipse, style=bold];\n", node.address(),
                        node.kind().name());
            break;

        case lexy::traverse_event::exit:
            // Now we can add all the connections.
            for (auto child : node.children())
                std::printf("\"node-%p\" -- \"node-%p\";\n", node.address(), child.address());
            break;

        case lexy::traverse_event::leaf:
            std::printf("\"node-%p\" [label=\"", node.address());
            if (node.lexeme().empty())
            {
                std::printf("%s", node.kind().name());
                std::puts("\", shape=box];");
            }
            else
            {
                for (auto c : node.lexeme())
                {
                    if (c == '"')
                        std::fputs(R"(\")", stdout);
                    else if (c == ' ')
                        std::fputs("␣", stdout);
                    else if (c == '\n')
                        std::fputs("⏎", stdout);
                    else if (c == '\\')
                        std::fputs("\\\\", stdout);
                    else if (std::iscntrl(c))
                        std::printf("0x%02X", unsigned(c) & 0xFF);
                    else
                        std::putchar(c);
                }
                std::puts("\", shape=box, style=filled];");
            }
            break;
        }
    }
    std::puts("}");

    if (result.is_success())
        return 0;
    else if (result.is_recovered_error())
        return 1;
    else if (result.is_fatal_error())
        return 2;
}

