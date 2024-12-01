#!/bin/sh
aws s3 cp --acl public-read --cache-control "max-age=90" index.html s3://veralin.dk/
aws s3 cp --acl public-read --cache-control "max-age=90" music.html s3://veralin.dk/
aws s3 cp --acl public-read --cache-control "max-age=90" wikimon.html s3://veralin.dk/
aws s3 cp --acl public-read --cache-control "max-age=90" ../../codes/gpujam.html s3://veralin.dk/
