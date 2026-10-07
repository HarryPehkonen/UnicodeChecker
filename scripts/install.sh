#!/usr/bin/env bash
#
# install — split out of the old tools/ci.sh (2026-10-06, card t_075c0a6f).
#
# Called from gate.toml as `[stage.install] cmd = "scripts/install.sh"`. The verdict
# vocabulary (ci_begin/ci_pass/ci_fail/ci_skip) is defined in scripts/gate-env.sh.
set -uo pipefail
. "$(dirname "$0")/gate-env.sh"

run_stage() {
    ci_begin "install (the default prefix, and what lands in <prefix>/bin)"
    if [ -z "${HOME:-}" ]; then
        ci_skip install "HOME is unset, so the default prefix has no value to be checked against"
        return 0
    fi
    if [ ! -x "$CI_BUILD_DIR/unicode_checker" ]; then
        ci_skip install "$CI_BUILD_DIR/unicode_checker is not built (the build stage runs first)"
        return 0
    fi

    local probe configured
    probe=$(mktemp -d)
    RUN_TMP_DIRS+=("$probe")
    if ! cmake -S "$REPO_ROOT" -B "$probe/build" > "$CI_LOG_DIR/install-configure.log" 2>&1; then
        ci_fail install "a plain configure failed — see $CI_LOG_DIR/install-configure.log" "$CI_LOG_DIR/install-configure.log"
    fi
    configured=$(sed -n 's/^CMAKE_INSTALL_PREFIX:PATH=//p' "$probe/build/CMakeCache.txt")
    if [ "$configured" != "$HOME/.local" ]; then
        printf '    a plain configure defaults the prefix to %s\n' "$configured"
        ci_fail install "the default prefix is '$configured', not '$HOME/.local' — otherwise a bare install goes to a system directory. Override deliberately: -DCMAKE_INSTALL_PREFIX=… at configure time, or cmake --install --prefix … at install time"
    fi
    printf '    default prefix: %s\n' "$configured"

    # Re-configure before installing. `cmake --install` runs build/cmake_install.cmake, which
    # was written by the LAST configure -- so a build dir that predates a CMakeLists change
    # would install the OLD rule and this stage would certify a rule that no longer exists.
    # Same stale-cache class as `make <new-target>` failing because the build dir never saw
    # the target. Measured here: with the destination sabotaged, a standalone `scripts/gate.sh
    # install` passed until this line re-configured the tree.
    if ! cmake -S "$REPO_ROOT" -B "$CI_BUILD_DIR" > "$CI_LOG_DIR/install-reconfigure.log" 2>&1; then
        ci_fail install "re-configuring $CI_BUILD_DIR failed — see $CI_LOG_DIR/install-reconfigure.log" "$CI_LOG_DIR/install-reconfigure.log"
    fi

    local prefix
    prefix=$(mktemp -d)
    RUN_TMP_DIRS+=("$prefix")
    if ! cmake --install "$CI_BUILD_DIR" --prefix "$prefix" > "$CI_LOG_DIR/install.log" 2>&1; then
        ci_fail install "cmake --install failed — see $CI_LOG_DIR/install.log" "$CI_LOG_DIR/install.log"
    fi
    if [ ! -x "$prefix/bin/unicode_checker" ]; then
        printf '    what the install rule wrote:\n'
        (cd "$prefix" && find . -type f) | sed 's/^/      /'
        ci_fail install "nothing executable landed at <prefix>/bin/unicode_checker — the install rule and CMAKE_INSTALL_BINDIR disagree"
    fi

    local built reported
    built=$("$CI_BUILD_DIR/unicode_checker" --version 2>&1 | head -1)
    reported=$("$prefix/bin/unicode_checker" --version 2>&1 | head -1)
    if [ "$reported" != "$built" ]; then
        ci_fail install "the installed binary reports '$reported' where the built one reports '$built'"
    fi
    # A binary that only works inside its build tree is parked, not installed.
    if ! "$prefix/bin/unicode_checker" "$REPO_ROOT/examples/01-plain-ascii.txt" > "$CI_LOG_DIR/install-run.log" 2>&1; then
        ci_fail install "the installed binary failed on examples/01-plain-ascii.txt — see $CI_LOG_DIR/install-run.log" "$CI_LOG_DIR/install-run.log"
    fi
    printf '    %s installed into <prefix>/bin and runs from there\n' "$reported"
    ci_pass install
}

run_stage "$@"
