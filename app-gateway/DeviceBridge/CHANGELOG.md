# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.3] - 2023-05-10
### Changed
- Fix `connectionStatusChanged` event:
  - USB interface availability will now be checked upon WS disconnection to prevent
    `BOOTING` reason before `DISCONNECTED`

## [1.0.2] - 2023-04-26
### Changed
- Fix `connectionStatusChanged` event and connection state flags:
  - `usbInterface` flag will now be set to false if Avahi could not access USB interface
  - `connected` flag will be set to false when `usbInterface` is changed
  - Above changes will prevent setting `connected` flag when device has `BOOTING` reason

## [1.0.1] - 2023-02-27
### Changed
- Make `connectionStatusChanged` event more consistent:
  - Do not send redundant events (i.e. when nothing is changed)
  - Sometimes, it could happen that a device is marked as connected even if
    it is plugged out - it should not happen anymore as `connected` status
    is going to be coupled with `usbInterface` status
  - `reason` should not be set as `DISCONNECTED` when a device is plugged in
    but not authenticated yet

## [1.0.0] - 2022-07-20
### Added
- Add CHANGELOG

### Change
- Reset API version to 1.0.0
- Change README to inform how to update changelog and API version
