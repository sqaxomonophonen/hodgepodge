#!/bin/sh
AWS_PROFILE=sqx aws s3api list-objects-v2 --bucket veralin.dk --prefix music/ > music.json
