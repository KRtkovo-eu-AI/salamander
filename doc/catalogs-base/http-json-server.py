#!/usr/bin/env python3

from http.server import ThreadingHTTPServer, BaseHTTPRequestHandler
from pathlib import Path
import argparse
import json
import sys
from datetime import datetime, timezone


ROUTES = {}


class JsonFileHandler(BaseHTTPRequestHandler):
    server_version = "SimpleJsonHttpServer/1.0"

    def do_GET(self):
        self.handle_request(send_body=True)

    def do_HEAD(self):
        self.handle_request(send_body=False)

    def handle_request(self, send_body: bool):
        path = self.path.split("?", 1)[0]

        if path == "/favicon.ico":
            self.send_response(204)
            self.send_header("Content-Length", "0")
            self.end_headers()
            return

        json_file = ROUTES.get(path)

        if json_file is None:
            self.send_json_error(
                status_code=404,
                message="Unknown endpoint",
                extra={
                    "path": path,
                    "available_endpoints": sorted(ROUTES.keys())
                },
                send_body=send_body
            )
            return

        try:
            content = json_file.read_text(encoding="utf-8")

            # Validate that the file really contains valid JSON.
            # The original formatting is preserved in the response.
            json.loads(content)

            data = content.encode("utf-8")

            self.send_response(200)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.send_header("Content-Length", str(len(data)))
            self.send_header("Cache-Control", "no-store")
            self.end_headers()

            if send_body:
                self.wfile.write(data)

        except FileNotFoundError:
            self.send_json_error(
                status_code=500,
                message="Configured JSON file was not found",
                extra={"file": str(json_file)},
                send_body=send_body
            )

        except json.JSONDecodeError as exc:
            self.send_json_error(
                status_code=500,
                message="Configured file is not valid JSON",
                extra={
                    "file": str(json_file),
                    "error": str(exc)
                },
                send_body=send_body
            )

        except Exception as exc:
            self.send_json_error(
                status_code=500,
                message="Unexpected server error",
                extra={"error": str(exc)},
                send_body=send_body
            )

    def send_json_error(self, status_code: int, message: str, extra=None, send_body=True):
        body = {
            "status": "error",
            "message": message,
            "time_utc": datetime.now(timezone.utc).isoformat()
        }

        if extra:
            body.update(extra)

        data = json.dumps(body, indent=2, ensure_ascii=False).encode("utf-8")

        self.send_response(status_code)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(data)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()

        if send_body:
            self.wfile.write(data)

    def log_message(self, fmt, *args):
        print(f"{self.client_address[0]}:{self.client_address[1]} - {fmt % args}")


def parse_route_mapping(raw_value: str):
    if "=" not in raw_value:
        raise argparse.ArgumentTypeError(
            "Route mapping must be in format /endpoint=file.json"
        )

    endpoint, file_path = raw_value.split("=", 1)
    endpoint = endpoint.strip()
    file_path = file_path.strip()

    if not endpoint.startswith("/"):
        endpoint = "/" + endpoint

    if not file_path:
        raise argparse.ArgumentTypeError("JSON file path cannot be empty")

    return endpoint, Path(file_path).resolve()


def main():
    global ROUTES

    parser = argparse.ArgumentParser(
        description="Simple HTTP server that serves external JSON files."
    )

    parser.add_argument("--host", default="0.0.0.0")
    parser.add_argument("--port", type=int, default=8080)
    parser.add_argument(
        "--route",
        action="append",
        type=parse_route_mapping,
        required=True,
        help="Mapping in format /endpoint=file.json. Can be used multiple times."
    )

    args = parser.parse_args()

    ROUTES = dict(args.route)

    print("Configured routes:")
    for endpoint, file_path in ROUTES.items():
        print(f"  http://{args.host}:{args.port}{endpoint} -> {file_path}")

    httpd = ThreadingHTTPServer((args.host, args.port), JsonFileHandler)

    print(f"Serving HTTP on {args.host}:{args.port}")
    httpd.serve_forever()


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("Stopping server")
        sys.exit(0)