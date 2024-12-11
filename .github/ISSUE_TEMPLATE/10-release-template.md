---
name: Release Checklist
about: Use this template for managing the next minor release.
title: "Release [X.Y.0] Checklist"
labels: "is:todo"
---

# Pre Release / Prerequisites

- [ ] Create release project and milestone
- [ ] Manage open bugs
- [ ] Check CUDA/HIP/SYCL versions
  - [ ] Remove code for unsupported versions

# Step-by-Step

- [ ] Update `develop` branch
  - [ ] Update TPLs (TODO: add PR)
    - [ ] [gflags](https://github.com/gflags/gflags)
    - [ ] [googletest](https://github.com/google/googletest/)
    - [ ] [nlohmann_json](https://github.com/nlohmann/json)
  - [ ] Update Changelog using the unreleased changes from the wiki (TODO: add PR)
  - [ ] Change version tag in `CMakeLists.txt` to `main` (PR-tag) (TODO: add PR)
- [ ] Manually check building with the newest compiler versions
  - see the CI for the branch `check-latest` [![Build status](https://gitlab.com/ginkgo-project/ginkgo-public-ci/badges/check-latest/pipeline.svg)](https://gitlab.com/ginkgo-project/ginkgo-public-ci/-/pipelines?page=1&scope=all&ref=check-latest)
- [ ] Manually check packaging CI (until automated)
  - see the CI for the branch `spack-ci`  [![Build status](https://gitlab.com/ginkgo-project/ginkgo-public-ci/badges/spack-ci/pipeline.svg)](https://gitlab.com/ginkgo-project/ginkgo-public-ci/-/pipelines?page=1&scope=all&ref=spack-ci)
- [ ] Manually check threadsanitizer CI (until automated again)
- [ ] Merge `develop` into `main`, with merge commit (i.e. `--no--ff`) (TODO: add PR)
- [ ] Create gitb release with new tag `vX.Y.0` on `main`

# Post Release

- [ ] Announce the release:
  - [ ] LinkedIn
  - [ ] E-Mail lists: NADigest
- [ ] Add new version to package managers
  - [ ] spack: https://github.com/ginkgo-project/spack
  - [ ] vcpkg: https://github.com/ginkgo-project/conan-center-index
  - [ ] conan: https://github.com/ginkgo-project/vcpkg
- [ ] Publish release on zenodo
- [ ] Prepare `develop` for next release (TODO: add PR)
  - [ ] Revert (PR-tag) on `develop`
  - [ ] Update version on develop to `X.(Y+1).0`
- [ ] In addition, until 2026:
  - [ ] Create branch `master-release/X.Y.0` from `master`
  - [ ] Change version tag to `master`
  - [ ] Cherry-pick commits `vX.(Y-1).0..vX.Y.0` into `master-release/X.Y.0`
  - [ ] Create PR to merge `master-release/X.Y.0` into `master` (TODO: add PR)
