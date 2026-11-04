#!/bin/sh
# Pin a downloaded source to REF and apply the patches in PATCH_DIR on top of it, as commits.
# Usage: apply_patches.sh REPO_DIR REF PATCH_DIR
#
# Runs on every build, and does nothing while REF, the patches and HEAD are unchanged, so
# file mtimes stay put and colcon does not recompile. It fails rather than build without a
# patch. A patch that stops applying after REF moves has usually been merged upstream.
set -eu

repo=$1
ref=$2
patch_dir=$(cd "$3" && pwd)
stamp=$(git -C "$repo" rev-parse --absolute-git-dir)/libmicroros-patches

fingerprint=$( { echo "$ref"; cat "$patch_dir"/*.patch; } | sha256sum | cut -d' ' -f1)

if [ -f "$stamp" ] && [ "$(cat "$stamp")" = "$fingerprint $(git -C "$repo" rev-parse HEAD)" ]; then
    exit 0
fi

if [ -n "$(git -C "$repo" status --porcelain --untracked-files=no)" ]; then
    echo "apply_patches: $repo has local changes; commit or discard them first" >&2
    exit 1
fi

rm -f "$stamp"
git -C "$repo" checkout --quiet --detach "$ref"
for patch in "$patch_dir"/*.patch; do
    if ! git -C "$repo" -c user.name=libmicroros -c user.email=libmicroros@localhost \
        am --quiet "$patch"; then
        git -C "$repo" am --abort 2>/dev/null || true
        echo "apply_patches: $patch does not apply on $ref" >&2
        exit 1
    fi
done
echo "$fingerprint $(git -C "$repo" rev-parse HEAD)" > "$stamp"
