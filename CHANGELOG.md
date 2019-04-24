# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/)
and this project adheres to [Semantic Versioning](http://semver.org/spec/v2.0.0.html).

## [Unreleased]

- Used specific versions of external dependencies.
- Fixed build on mac and centos.
- Fix the strerror_r() usage for all cases.
- Replace malloc/strcpy with strdup
- Re-enabled the temporarily commented out setting that prevented cyclic building
- Switched from nanomsg (Release 1.1.2) to NNG (Release v1.0.1)
- Revert from NNG
- Update to use nanomsg version 1.1.4

## [1.0.0] - 2018-06-19
### Added
- Initial stable release.

## [0.0.1] - 2017-01-13
### Added
- Initial creation

[Unreleased]: https://github.com/Comcast/libparodus/compare/1.0.0...HEAD
[1.0.0]: https://github.com/Comcast/libparodus/compare/3d57eb64e06ff9cb363bec8142e728c50231f5e8...1.0.0
