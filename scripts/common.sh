# shared helpers (sourced)
MOGLI_CACHE="${MOGLI_CACHE:-$HOME/.cache/mogli-build}"
mkdir -p "$MOGLI_CACHE/dl"
fetch_verified() { # url file sha256
  local url=$1 file=$MOGLI_CACHE/dl/$2 sha=$3
  [ -f "$file" ] || curl -fsSL -o "$file" "$url"
  echo "$sha  $file" | sha256sum -c - || { echo "sha256 mismatch for $file" >&2; rm -f "$file"; exit 1; }
}
