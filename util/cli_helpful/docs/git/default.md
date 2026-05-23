
### Add all and commit at the same time
```sh
git -am : add and commit
```

### Force merge two repositories with completely different commit histories. (merge history) - becareful
```sh
git pull origin main --allow-unrelated-histories
```

### Set new remote
```sh
# Check:
git remote -
git remote set-url origin https://github.com/<owner_name>/<repo_name>.git
# OR: git remote set-url origin git@github.com:<owner_name>/<repo_name>.git
```

### Update submodules
```sh
git submodule update --init --recursive
```

```sh
git submodule add https://github.com/<owner_name>/<repo_name>.git
```

```sh
git reset --soft HEAD~1
```