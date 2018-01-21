#include "route.h"

void UrlMap::register_rule(const ByteBuffer& rule, UrlMap::RequestHandler&& handler, const std::vector<HttpMethod>& methods)
{
    if (rule.size() == 0 || rule[0] != '/') {
        throw InvalidUrlRule("path is not absolute");
    }

    UrlEntry* entry = &root;
    size_t begin_index = 1;
    for (size_t i = 1; i <= rule.size(); i++) {
        if (rule[i] == '/' || i == rule.size()) {
            ByteBuffer part(rule.data() + begin_index, i - begin_index);

            if (i == rule.size() && part.size() == 0) break; /* trailing slash */

            if (part[0] == ':') { /* pattern */
                entry->pattern_name = ByteBuffer(part.data() + 1, part.size() - 1);
                if (entry->pattern_name.size() == 0) {
                    throw InvalidUrlRule("empty pattern");
                }

                entry->pattern_entry = std::make_unique<UrlEntry>();
                entry = entry->pattern_entry.get();
            } else {
                auto it = entry->children.find(part);
                if (it == entry->children.end()) {
                    auto new_entry = std::make_unique<UrlEntry>();
                    entry->children[part] = std::move(new_entry);
                }
                entry = entry->children[part].get();
            }

            begin_index = i + 1;
        }
    }

    for (auto mth : methods) {
        entry->handlers[mth] = handler;
    }
}

UrlMap::RequestHandler UrlMap::match_url(const ByteBuffer& url, HttpMethod method, UrlPatternMap& pattern_map)
{
    if (url.size() == 0 || url[0] != '/') {
        throw UnmatchedUrl("path is not absolute");
    }

    UrlEntry* entry = &root;
    size_t begin_index = 1;
    for (size_t i = 1; i <= url.size(); i++) {
        if (url[i] == '/' || i == url.size()) {
            ByteBuffer part(url.data() + begin_index, i - begin_index);

            if (i == url.size() && part.size() == 0) break; /* trailing slash */

            if (entry->pattern_entry) { /* pattern */
                pattern_map[entry->pattern_name] = part;
                entry = entry->pattern_entry.get();
            } else {
                auto it = entry->children.find(part);
                if (it == entry->children.end()) {
                    throw UnmatchedUrl("no handler for this URL");
                }

                entry = it->second.get();
            }
            begin_index = i + 1;
        }
    }

    auto it = entry->handlers.find(method);
    if (it == entry->handlers.end()) {
        throw UnmatchedUrl("no handler for this method");
    }

    return it->second;
}
