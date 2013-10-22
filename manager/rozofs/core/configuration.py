# -*- coding: utf-8 -*-
#
# Copyright (c) 2013 Fizians SAS. <http://www.fizians.com>
# This file is part of Rozofs.
#
# Rozofs is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published
# by the Free Software Foundation, version 2.
#
# Rozofs is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see
# <http://www.gnu.org/licenses/>.

from rozofs.core.libconfig import config_t, config_read_file, \
    config_destroy, config_setting_t, CONFIG_TYPE_GROUP, config_value_t, \
    config_write_file, CONFIG_TRUE
import time
import shutil

class ConfigurationParser():

    def parse(self, configuration, config):
        raise NotImplementedError()

    def unparse(self, config, configuration):
        raise NotImplementedError()


class ConfigurationReader(object):

    def __init__(self, fname, parser):
        self._file = fname
        self._parser = parser

    def read(self, configuration):
        c = config_t()
        if (config_read_file(c, self._file) != CONFIG_TRUE):
            error_text = c.error_text
            config_destroy(c)
            raise Exception(error_text)
        try:
            self._parser.unparse(c, configuration)
        finally:
            config_destroy(c)

        config_destroy(c)
        return configuration


class ConfigurationWriter(object):

    def __init__(self, fname, parser):
        self._file = fname
        self._parser = parser

    def write(self, configuration):
        backup = "%s.%s" % (self._file, time.strftime("%Y%m%d-%H:%M:%S", time.localtime()))
        shutil.copy(self._file, backup)
        c = config_t()
        c.root = config_setting_t()
        c.root.type = CONFIG_TYPE_GROUP
        c.root.value = config_value_t()
        c.root.config = c

        try:
            self._parser.parse(configuration, c)
        except Exception as e:
            config_destroy(c)
            raise e

        if (config_write_file(c, self._file) != CONFIG_TRUE):
            error_text = c.error_text
            config_destroy(c)
            raise Exception(error_text)

        config_destroy(c)
