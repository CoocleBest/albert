// Copyright (c) 2022 Manuel Schneider

#include "itemindex.h"
#include "albert/logging.h"
#include "albert/extensions/queryhandler.h"
#include "queryengine.h"
using namespace albert;
using namespace std;
static const char *CFG_TRIGGER = "trigger";
static const char* CFG_ERROR_TOLERANCE_DIVISOR = "error_tolerance_divisor";
static const uint DEF_ERROR_TOLERANCE_DIVISOR = 3;
static const char* CFG_CASE_SENSITIVE = "case_sensitive";
static const bool  DEF_CASE_SENSITIVE = false;
static const char* CFG_SEPARATORS = "separators";
static const char* DEF_SEPARATORS = R"R([\s\\\/\-\[\](){}#!?<>"'=+*.:,;_]+)R";
static const uint GRAM_SIZE = 3;


QueryEngine::QueryEngine(ExtensionRegistry &registry) :
        ExtensionWatcher<QueryHandler>(registry),
        global_search_handler(registry)
{
}

std::unique_ptr<albert::Query> QueryEngine::query(const QString &query_string)
{
    unique_ptr<::Query> query;

    for (const auto &[trigger, handler] : trigger_map)
        if (query_string.startsWith(trigger))
            query = make_unique<::Query>(ExtensionWatcher<QueryHandler>::extensions(), *handler,
                                         query_string.mid(trigger.size()), trigger);

    if (!query)
        query = make_unique<::Query>(ExtensionWatcher<QueryHandler>::extensions(),
                                     global_search_handler, query_string);

    return query;
}

void QueryEngine::updateTriggers()
{
    trigger_map.clear();

    for (auto [id, handler]: extensionRegistry().extensionsOfType<QueryHandler>()) {
        auto trigger = handler->allow_trigger_remap()
                       ? handler->settings()->value(CFG_TRIGGER, handler->default_trigger()).toString()
                       : handler->default_trigger();

        if (trigger.isEmpty()) {
            WARN << QString("Triggers must not be empty: %1.").arg(handler->id());
            continue;
        }

        const auto &[it, success] = trigger_map.emplace(trigger, handler);
        if (!success)
            WARN << QString("Trigger conflict '%1': Already reserved for %2.").arg(trigger, it->second->id());
    }
}

void QueryEngine::onAdd(QueryHandler *handler)
{
    updateTriggers();
}

void QueryEngine::onRem(QueryHandler *handler)
{
    updateTriggers();
}

void QueryEngine::onAdd(albert::IndexQueryHandler *handler)
{
    auto s = handler->settings();
    auto *index = new ItemIndex(
        s->value(CFG_SEPARATORS, DEF_SEPARATORS).toString(),
        s->value(CFG_CASE_SENSITIVE, DEF_CASE_SENSITIVE).toBool(),
        GRAM_SIZE,
        s->value(CFG_ERROR_TOLERANCE_DIVISOR, DEF_ERROR_TOLERANCE_DIVISOR).toUInt()
    );
    handler->setIndex(unique_ptr<albert::Index>{index});
}
