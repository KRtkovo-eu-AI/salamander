# Optional local AI command provider

`salamatrix_ai_local.py` is an opt-in adapter for the provider-neutral
`Salamatrix.AI` command contract. It reads one UTF-8 JSON request from stdin and
writes one JSON response to stdout, so it can be selected with the existing
`SALAMATRIX_AI_COMMAND` setting without linking a model SDK into Salamander.

## Ollama

```text
SALAMATRIX_AI_COMMAND=python.exe "<path>\\salamatrix_ai_local.py"
SALAMATRIX_AI_ENDPOINT=http://127.0.0.1:11434/api/generate
SALAMATRIX_AI_MODEL=llama3.2
SALAMATRIX_AI_SYSTEM=Use only the declared Salamander APIs and prefer read-only effects.
```

## llama.cpp server

```text
SALAMATRIX_AI_COMMAND=python.exe "<path>\\salamatrix_ai_local.py"
SALAMATRIX_AI_PROTOCOL=chat-completions
SALAMATRIX_AI_ENDPOINT=http://127.0.0.1:8080/v1/chat/completions
SALAMATRIX_AI_MODEL=local-model
```

The wrapper is not launched, and it does not make a network request, unless
`SALAMATRIX_AI_COMMAND` explicitly selects it. Its endpoint timeout is capped
at 110 seconds; the Automation host applies its own two-minute provider limit.
The host still validates the returned script, capabilities, and effect flags
before it can be previewed, run, or saved as an extension package.

The offline contract checks are in
`../tests/salamatrix_ai_local_tests.py`; they mock the HTTP transport and never
contact a model server.
