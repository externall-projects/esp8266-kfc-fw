/**
 * Author: sascha_lammers@gmx.de
 */

var webUIComponent = {

    container: $('#webui'),

    defaults: {
        row: {
            columns_per_row: 0
        },
        column: {
            group: { columns: 12 },
            switch: { min: 0, max: 1, columns: 2, zero_off: true, display_name: false, attributes: ['min', 'max', 'value', 'zero-off', 'display-name' ] },
            slider: { min: 0, max: 255, columns: 12, attributes: ['min', 'max', 'zero-off', 'value' ] },
            color_slider: { min: 15300, max: 50000 },
            sensor: { columns: 3 },
            binary_sensor: { columns: 2 },
        }
    },

    groups: [],
    pendingRequests: [],
    lockPublish: false,

    prepareOptions: function(options) {
        // apply default settings to all columns and calculate number of columns

        options = $.extend({}, this.defaults.row, options);

        var self = this;
        $(options.columns).each(function() {
            var defaults = self.defaults.column[this.type];
            if (this.type === "slider" && this.color) {
                $.extend(defaults, self.defaults.column.color_slider);
            }
            var tmp = $.extend({}, defaults, this);
            $.extend(this, tmp);
            options.columns_per_row += this.columns;
        });

        self.row = options;
        return options;
    },

    copyAttributes: function(element, options) {
        if (options.attributes.length) {
            $(options.attributes).each(function() {
                var idx = this.replace('-', '_');
                if (options[idx] !== undefined) {
                    // console.log("copyAttributes", this, options[idx]);
                    element.attr(this, options[idx]);
                }
            });
        }
    },

    publishState: function(id, value, type) {
        // type = state, value 0 or 1
        // type = value

        if (window._webui_debug) console.log("publish state", id, value, type, this.lockPublish);//, this.pendingRequests[id]);
        if (this.lockPublish) {
            return;
        }

        var pendingRequest = this.pendingRequests[id];
        if (pendingRequest && pendingRequest.pending) {

            pendingRequest.value = value;
            pendingRequest.type = type;

        } else {
            this.pendingRequests[id] = {
                pending: true,
                value: value,
                type: type,
            };
            this.socket.send('+' + type + ' ' + id + ' ' + value);

            var self = this;
            window.setTimeout(function() { // max. update rate 100ms, last published state will be sent after the timeout
                self.pendingRequests[id].pending = false;
                if (self.pendingRequests[id].value != value || self.pendingRequests[id].type != type) {
                    self.publishState(id, self.pendingRequests[id].value, self.pendingRequests[id].type)
                }
            }, 100);
        }

    },

    toggleGroup: function(groupId, value) {
        if (window._webui_debug) console.log("toggleGroup", groupId, value);

        if (this.groups[groupId].switch.value == value) {
            return;
        }

        var self = this;
        $(this.groups[groupId].components).each(function() {
            if (this.type == 'switch' || this.type == 'slider') {
                if (this.zero_off) {
                    self.publishState(this.id, value, 'set_state');
                }
            }
        });
    },

    moveSlider: function(slider, id, value) {
        this.publishState(id, value, 'set');
        this.updateGroup(id, value);
        this.updateGroupSwitch();
    },

    sliderCallback: function (slider, position, value, onSlideEnd) {

        var element = $(slider.$element);
        var dataId = element.attr('id');

        if (dataId.substring(0, 12) == 'group_switch') {
            var groupId = parseInt(dataId.substring(13)) - 1;
            this.toggleGroup(groupId, value);
        }
        else {
            this.moveSlider(slider, dataId, value);
        }
        this.sliderUpdateCSS(element, slider.$handle, slider.$fill);
    },

    sliderUpdateCSS: function(element, handle, fill) {
        if (element.attr('zero-off')) {
            var value = element.val();

            // change brightness of slider
            var a = (value / parseInt(element.attr('max')) * 0.7);
            fill.css('background-image', '-webkit-gradient(linear, 50% 0%, 50% 100%, color-stop(0%, rgba(255, 255, 255, '+a+')), color-stop(100%, rgba(255, 255, 255, '+a+')))');
            fill.css('background-image', '-moz-linear-gradient(rgba(255, 255, 255, '+a+'), rgba(255, 255, 255, 0, '+a+'))');
            fill.css('background-image', '-webkit-linear-gradient(rgba(255, 255, 255, '+a+'), rgba(255, 255, 255, '+a+'))');
            fill.css('background-image', 'linear-gradient(rgba(255, 255, 255, '+a+'), rgba(255, 255, 255, '+a+'))');

            // change color of handle
            if (value == 0) {
                handle.removeClass('on').addClass('off');
            } else if (value) {
                handle.removeClass('off').addClass('on');
            }
        }

    },

    addToGroup: function(options) {
        if (this.groups.length) {
            var group = this.groups[this.groups.length - 1];
            if (options.group_switch) {
                group.switch = options;
            }
            else if (group.switch) {
                group.components.push(options);
                if (!group.switch.state) {
                    if (options.zero_off) {
                        if (options.state) {
                            group.switch.state = true;
                            group.switch.value = 1;
                            $('#' + group.switch.id).val(1);
                        }
                    }
                }
            }
        }
    },

    createColumn: function (options, innerElement) {
        var containerClass = 'webuicomponent';
        if (this.row.extra_classes) {
            containerClass += ' ' + this.row.extra_classes;
        }
        var html = '';
        for(i = 0; i < innerElement.length; i++) {
            html += innerElement[i].outerHTML;
        }
        return $('<div class="' + containerClass + '">' + html + '</div>');
    },

    addElement: function (options) {
        if (options.type === "group") {
            this.groups.push({components: []});
            var template = '<div class="row group"><div class="col"><h1>' + options.name + '</h1></div>';
            if (options.has_switch) {
                var o = {
                    type: 'switch',
                    value: 0,
                    id: 'group_switch_' + this.groups.length,
                    group_switch: true
                };
                o = $.extend(o, this.defaults.column.switch);
                template += '<div class="col">' + this.addElement(o).html() + '</div>';
            }
            template += '</div>';
            var element = this.createColumn(options, $(template));
            return element;
        }
        else if (options.type === "switch") {
            if (options.display_name) {
                var element = $('<div class="named-switch"><div class="row"><div class="col"><input type="range" id=' + options.id + '></div></div><div class="row"><div class="col switch-name">' + options.name + '</div></div></div>');
            } else {
                var element = $('<div class="switch"><input type="range" id=' + options.id + '></div>');
            }
            var input = element.find("input");
            this.copyAttributes(input, options)
            var element = this.createColumn(options, element);
            this.addToGroup(options);
            return element;
        }
        else if (options.type === "slider") {
            var slider = $('<div class="' + ((options.color === "temperature") ? 'color-slider' : 'slider') + '"><input type="range" id=' + options.id + '></div>');
            var input = slider.find("input");
            this.copyAttributes(input, options)
            var element = this.createColumn(options, slider);
            this.addToGroup(options);
            return element;
        }
        else if (options.type === "sensor" || options.type === "binary_sensor") {
            var element;
            if (options.value === undefined || options.state === false) {
                options.value = 'N/A';
            }
            if (options.render_type == 'badge') {
                var element = this.createColumn(options, $('<div class="badge-sensor"><div class="row"><div class="col"><div class="outer-badge"><div class="inner-badge"><h4 id="' + options.id + '">' + options.value + '</h4><div class="unit">' + options.unit + '</div></div></div></div></div><div class="row"><div class="col text-center">' + options.name + '</div></div></div>'));
            }
            else if (options.render_type == 'wide') {
            }
            else if (options.render_type == 'medium') {
            }
            else {
                var element = this.createColumn(options, $('<div class="sensor"><h3>' + options.name + '</h3><h1><span id="' + options.id + '">' + options.value + '</span><span class="unit">' + options.unit + '</span></h1>'));
            }
            // this.addToGroup(options);
            return element;
        }
        else if (options.type === "row") {

            options = this.prepareOptions(options);

            var rowClass = 'row';
            if (options.align === "center") {
                rowClass += ' justify-content-center';
            }
            var html = '<div class="' + rowClass + '">';
            var self = this;
            $(options.columns).each(function(idx) {
                var colClass = 'col-md';
                if (this.columns) {
                    colClass += '-' + this.columns;
                }
                // add extra column to align other columns on the right
                if (idx == 0 && options.align === "right" & options.columns_per_row < 12) {
                    colClass += ' offset-md-' + (12 - options.columns_per_row);
                }
                html += '<div class="' + colClass +'">';
                html += self.addElement(this)[0].outerHTML;
                html += '</div>';
            });
            // add extra column to fill the row
            if (options.columns_per_row < 12 && options.align === "left") {
                html += '<div class="offset-md-' + (12 - options.columns_per_row) + '">';
                html += '</div>';
            }
            html += '</div>';
            this.container.append(html);

        }
        else {
            console.log("Unknown type");
            console.log(options);
        }
    },

    updateUI: function(json) {

        this.lockPublish = true;

        this.groups = [];
        this.pendingRequests = [];
        this.container.html('');

        var self = this;
        $(json).each(function() {
            self.addElement(this);
        });
        this.container.append('<br><br>');

        this.container.find('input[type="range"]').each(function() {
            var $this = $(this);
            var options = {
                polyfill : false,
                onSlideEnd: function(position, value) {
                    self.sliderCallback(this, position, value, true);
                }
            };
            if ($this.attr('min') != 0 || $this.attr('max') != 1) {
                options.onSlide = function(position, value) {
                    self.sliderCallback(this, position, value, false);
                }
            }
            self.updateSliderCSS($this.rangeslider(options));
        });

        this.lockPublish = false;
    },

    updateSliderCSS: function(input) {
        var next = input.next();
        var handle = next.find('.rangeslider__handle');
        var fill = next.find('.rangeslider__fill');
        this.sliderUpdateCSS(input, handle, fill);
    },

    // update state of components
    updateGroup: function(id, value) {
        $(this.groups).each(function() {
            $(this.components).each(function() {
                if (this.id == id && this.zero_off) {
                    this.state = value != 0;
                }
            });
        });
    },

    // check if any component is on and update group switch
    updateGroupSwitch: function() {
        var self = this;
        $(this.groups).each(function() {
            var value = 0;
            $(this.components).each(function() {
                if (this.state && this.zero_off) {
                    value = 1;
                }
            });
            if (this.switch && this.switch.value != value) {
                var element = $('#' + this.switch.id);
                if (element.length) {
                    this.switch.value = value;
                    element.val(value).change();
                    self.updateSliderCSS(element);
                }
            }
        });
    },

    updateEvents: function(events) {
        if (window._webui_debug) console.log("updateEvents", events);
        this.lockPublish = true;
        var self = this;
        $(events).each(function() {
            var element = $('#' + this.id);
            if (element.length) {
                if (this.value !== undefined) {
                    self.updateGroup(this.id, this.value);
                    if (element[0].nodeName != 'INPUT') {
                        // console.log("update_event", "innerHtml", this);
                        element.html(this.value);
                    } else {
                        // console.log("update_event", "input", this);
                        element.val(this.value).change();
                        self.updateSliderCSS(element);
                    }
                }
                if (this.state !== undefined) {
                    self.updateGroup(this.id, this.state ? 1 : 0);
                }
            }
        });
        this.updateGroupSwitch();
        this.lockPublish = false;
    },

    requestUI: function() {
        var url = $.getHttpLocation('/webui_get')
        var SID = $.getSessionId();
        var self = this;
        $.get(url + '?SID=' + SID, function(data) {
            if (window._webui_debug) console.log(data);
            self.updateUI(data.data);
            self.updateEvents(data.values);
        }, 'json');
    },

    socketHandler: function(event) {
        if (event.data) {
            var json = JSON.parse(event.data);
            if (window._webui_debug) console.log("socket_handler", event, json)
            if (json.type === 'ue') {
                this.updateEvents(json.events);
            }
        } else if (event.type == 'auth') {
            //event.socket.send('+GET_VALUES');
            this.requestUI();
        }
    },

    init: function() {
        var url = $.getWebSocketLocation('/webui_ws');
        var SID = $.getSessionId();
        var self = this;
        this.socket = new WS_Console(url, SID, 1, function(event) {
            event.self = self;
            self.socketHandler(event);
        } );

        if (window._webui_debug) {
            $('body').append('<textarea id="console" style="width:270px;height:70px;font-size:10px;font-family:consolas;z-index:999;position:fixed;right:5px;bottom:5px"></textarea>');
            this.socket.setConsoleId("console");
        }

        this.socket.connect();
    }
};
