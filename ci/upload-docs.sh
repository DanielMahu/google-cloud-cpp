#!/usr/bin/env bash
#
# Copyright 2017 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set -eu

if [ "${TRAVIS_PULL_REQUEST}" != "false" ]; then
  echo "Skipping document generation as it is disabled for pull requests."
  exit 0
fi

if [ "${GENERATE_DOCS:-}" != "yes" ]; then
  echo "Skipping document generation as it is disabled for this build."
  exit 0
fi

if [ "${TRAVIS_BRANCH:-}" != "master" ]; then
  echo "Skipping document generation as it is disabled for non-master branches."
  exit 0
fi

# The usual way to host documentation in ${GIT_NAME}.github.io/${PROJECT_NAME}
# is to create a branch (gh-pages) and post the documentation in that branch.
# We first do some general git configuration:

# Clone the gh-pages branch into the doc/html subdirectory.
readonly REPO_URL=$(git config remote.origin.url)
git clone -b gh-pages "${REPO_URL}" doc/html

# Remove any previous content of the branch.  We will recover any unmodified
# files in a second.
(cd doc/html && git rm -qfr --ignore-unmatch .)

# Copy the build results out of the Docker image.
readonly IMAGE="cached-${DISTRO}-${DISTRO_VERSION}"
sudo docker run --volume "$PWD/doc:/d" --rm -it "${IMAGE}:tip" cp -r /var/tmp/build/gccpp/bigtable/doc/html /d

cd doc/html
git config user.name "Travis Build Robot"
git config user.email "nobody@users.noreply.github.com"
git add --all .

if git diff --exit-code; then
  echo "Skipping documentation upload as there are no differences to upload."
  exit 0
fi

git commit -q -m"Automatically generated documentation"

if [ "${REPO_URL:0:8}" != "https://" ]; then
  echo "Repository is not in https:// format, attempting push to ${REPO_URL}"
  git push
  exit 0
fi

if [ -z "${GH_TOKEN:-}" ]; then
  echo "Skipping documentation upload as GH_TOKEN is not configured."
  exit 0
fi

readonly REPO_REF=${REPO_URL/https:\/\/}
git push https://${GH_TOKEN}@${REPO_REF} gh-pages
