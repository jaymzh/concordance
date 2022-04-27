# Release Process

## Check ABI/API version

The libtooling versioning has a variety of issues. We simply use a single
integer for our so-version and we bump it anytime we change the API for
safety (i.e. libconcord.h)

```
vi libconcord/Makefile.am libconcord/bindings/python/libconcord.py \
concordance/configure.ac concordance/win/concordance.nsi
```

## Bump concordance version number

```
vi concordance/concordance.c concordance/configure.ac \
libconcord/configure.ac libconcord/bindings/python/setup.py
```

## Check for old version:

```
fgrep -R '0.20' *
```

## Make clean, recompile, reinstall, test

## Change version in Changelog, commit

## Git tag release

```shell
version="1.1"
git tag -a v$version -m "Concordance version $version"
git push origin --tags
```

## Make tarbal

```
# from top-level
git archive --format=tar --prefix=concordance-$version/ v$version \
> /tmp/concordance-$version-prep.tar
cd /tmp && tar xf concordance-$version-prep.tar
cd /tmp/concordance-$version/libconcord
mkdir m4; autoreconf -i
cd ../concordance
mkdir m4; autoreconf -i
cd ../..
tar jcf concordance-$version.tar.bz2 concordance-$version
```

## test compile

## sign

```shell
gpg -ab /tmp/concordance-$version.tar.bz2
```

## Write up release notes

## Retrieve Windows binaries

They will be in the Actions run for the PR with the version bump.

## Upload tarballs and windows binaries

* To GitHub
* To Sourceforge

## Update website

vim:textwidth=78:
