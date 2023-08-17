#!/usr/bin/env bash

cp .github/bot-pr-base.sh /tmp
source /tmp/bot-pr-base.sh

echo -n "Run Pre-Commit checks"

git remote add fork "$HEAD_URL"
git fetch fork "$HEAD_BRANCH"
git fetch origin "$BASE_BRANCH"

# checkout current PR head
LOCAL_BRANCH=format-tmp-$HEAD_BRANCH
git checkout -b $LOCAL_BRANCH fork/$HEAD_BRANCH

pipx run pre-commit run --show-diff-on-failure --color=always --from-ref "origin/$BASE_BRANCH" --to-ref HEAD || true

echo -n "Collecting information on changed files"

# check for changed files, replace newlines by \n
LIST_FILES=$(git diff --name-only | sed '$!s/$/\\n/' | tr -d '\n')
echo -n .

git diff > /tmp/format.patch
mv /tmp/format.patch .
echo -n .

bot_delete_comments_matching "Error: The following files need to be formatted"
echo -n .

if [[ "$LIST_FILES" != "" ]]; then
  MESSAGE="The following files need to be formatted:\n"'```'"\n$LIST_FILES\n"'```'
  MESSAGE="$MESSAGE\nYou can find a formatting patch under **Artifacts** [here]"
  MESSAGE="$MESSAGE($JOB_URL) or run "'`format!` if you have write access to Ginkgo'
  bot_error "$MESSAGE"
fi
echo .
