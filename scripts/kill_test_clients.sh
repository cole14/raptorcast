#!/bin/bash

kill `ps | grep test_client | awk '{ print $1 }'`
