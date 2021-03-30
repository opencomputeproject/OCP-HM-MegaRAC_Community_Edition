
var g_bus;
var initial_state = {
    'bus': 'session',
};

function populate_tree_services(node, cb, bus)
{
    jQuery.getJSON('/bus/' + bus.name, function(data) {
            var children = [];
            for (var i in data.objects) {
                var service = data.objects[i];
                if (service.name[0] == ':')
                        continue;
                children.push({
                        'text': service.name,
                        'children': true,
                        'type': 'service',
                        'bus': bus,
                        'service': service,
                    });
            }
            cb(children);
    });
}

function populate_tree_objects(node, cb)
{
    var ctx = node.original;
    jQuery.getJSON('/bus/' + ctx.bus.name + '/' + ctx.service.name,
            function(data) {
                var children = [];
                for (var i in data.objects) {
                    var obj = data.objects[i];
                    children.push({
                            'text': obj.path,
                            'children': true,
                            'type': 'object',
                            'bus': ctx.bus,
                            'service': ctx.service,
                            'object': obj,
                        });
                }
                cb(children);
    });
}

function populate_tree_interfaces(node, cb)
{
    var ctx = node.original;
    jQuery.getJSON('/bus/' + ctx.bus.name + '/' + ctx.service.name +
                ctx.object.path,
            function(data) {
                console.log(data);
                var children = [];
                for (var i in data.interfaces) {
                    var iface = data.interfaces[i];
                    children.push({
                            'text': iface.name,
                            'children': true,
                            'type': 'interface',
                            'bus': ctx.bus,
                            'service': ctx.service,
                            'object': ctx.object,
                            'interface': iface,
                        });
                }
                cb(children);
    });
}

function format_one_type(type)
{
    var ret = Object();
    var basic_types = {
        'v': 'variant',
        'i': 'int32',
        'x': 'int64',
        'u': 'uint32',
        't': 'uint64',
        'g': 'type',
        'h': 'file',
        'd': 'double',
        's': 'string',
        'o': 'objpath',
        'y': 'byte',
        'b': 'boolean',
    }

    var c = type[0];
    if (basic_types[c]) {
        ret.type = basic_types[c];
        ret.left = type.substring(1);
        return ret;
    }

    if (c == '(') {
        var i;
        var n;
        for (i = 1, n = 1; i < type.length; i++) {
            var c = type[i];
            if (c == '(')
                n++;
            if (c == ')')
                if (--n == 0)
                    break;

        }
        if (i == type.length)
            return ret;
        var content = type.substring(1, i);
        ret.type = 'struct {' + format_types(content).join(', ') + '}';
        ret.left = type.substring(i+1);
        return ret;
    }

    if (c == 'a') {
        var content = type.substring(1);
        if (content[0] == '{') {
            var i;
            var n;
            for (i = 1, n = 1; i < content.length; i++) {
                var c = content[i];
                if (c == '{')
                    n++;
                if (c == '}')
                    if (--n == 0)
                        break;

            }
            content = content.substring(1, i);
            var types = format_types(content);
            console.log("{", content, "}", types);
            ret.type = 'dict {' + types[0] + ' → ' + types[1] + '}';
            ret.left = type.substring(i+1);
        } else {
            var t = format_one_type(content);
            ret.type = '[' + t.type + ']';
            ret.left = t.left;
        }
        return ret;
    }

    ret.type = c;
    ret.left = type.substring(1);
    return ret;
      
}

function format_types(type)
{
    var t;
    var ts = [];
    for (;;) {
        t = format_one_type(type);
        ts.push(t.type);

        if (!t.left || !t.left.length)
            break;
        type = t.left;
    }
    return ts;
}

function format_type(type)
{
    return format_types(type)[0];
}

function format_method_arg(arg)
{
    return '<span class="method-arg">' +
            '<span class="method-arg-type">' + format_type(arg.type) +
             '</span> ' +
            '<span class="method-arg-name">' + arg.name + '</span></span>';
}

function format_method(method)
{
    var in_args = [];
    var out_args = [];
    for (var i in method.args) {
        var arg = method.args[i];
        var str = format_method_arg(arg);
        if (arg.direction == 'in')
            in_args.push(str);
        else
            out_args.push(str);
    }
    return '<span class="method">' +
              '<span class="method-name">' + method.name + "</span>" +
              '<span class="method-args">(' + in_args.join(', ') + ')</span>' +
              ' → ' +
              '<span class="method-args">' + out_args.join(', ') + '</span>' +
              '</span>'
}

function format_property(name, value)
{
    return '<span class="property">' +
            '<span class="property-name">' + name + '</span>' +
            ' = ' + 
            '<span class="property-value">' + value + '</span>' +
           '</span>'
}

function populate_tree_interface_items(node, cb)
{
    var ctx = node.original;
    jQuery.getJSON('/bus/' + ctx.bus.name + '/' + ctx.service.name +
                ctx.object.path + '/' + ctx.interface.name,
            function(data) {
                console.log(data);
                var children = [];
                for (var i in data.methods) {
                    var method = data.methods[i];
                    children.push({
                            'text': format_method(method),
                            'children': [],
                            'type': 'method',
                            'method': method,
                            'bus': ctx.bus,
                            'service': ctx.service,
                            'object': ctx.object,
                            'interface': ctx.interface,
                        });
                }
                for (var i in data.properties) {
                    var property = data.properties[i];
                    children.push({
                            'text': format_property(i, property),
                            'children': [],
                        });
                }
                cb(children);
    });
}
function populate_tree(node, cb)
{
    if (!node.original) {
        populate_tree_services(node, cb, g_bus);
        return;
    }

    var type = node.original.type;
    if (type == 'service') {
        populate_tree_objects(node, cb);
    } else if (type == 'object') {
        populate_tree_interfaces(node, cb);
    } else if (type == 'interface') {
        populate_tree_interface_items(node, cb);
    }
}

function select_node(evt, data)
{
    var ctx = data.node.original;
    if (!ctx || ctx.type != 'method')
        return;

    var method = ctx.method;
    jQuery("#method-call-ui").remove();

    var container = data.instance.get_node(data.node, true);

    var in_args = []
    for (var i in method.args) {
        var arg = method.args[i];
        if (arg.direction != 'in')
            continue;
        in_args.push({
            'type': format_method_arg(arg),
            'idx': i,
        });
    }
    method.nargs = in_args.length;

    var template = jQuery.templates('#method-call-ui-template');
    container.append(template.render({
                'method': method,
                'in_args': in_args,
    }));
    jQuery("form", container).submit(function(e) {
            var form = jQuery(e.target);
            var n = jQuery("input[name=argN]", form).val();
            var data = [];
            for (var i = 0; i < n; i++) {
                var val = jQuery("input[name=arg" + i + ']', form).val();
                try {
                    data.push(JSON.parse(val));
                } catch (e) {
                    data.push(null);
                }
            }
            jQuery.ajax(form.attr("action"),
                    {
                        'method': 'POST',
                        'contentType': 'text/json',
                        'data': JSON.stringify(data),
                        'dataType': 'text',
                        'success': function(data, status) {
                            jQuery('#method-call-ui-result').text(data);
                        },
                    });
            return false;
    });
}

function set_bus(b)
{
    g_bus = b;
    $('#container').jstree({
        'core': {
            'data': populate_tree,
            'multiple': false,
            'themes': {
                'icons': false,
            },
        }
    }).on('select_node.jstree', select_node);
}

function init()
{
    var select = jQuery('select#bus');
    select.empty();
    select.append(jQuery('<option>').text('---'));
    jQuery.getJSON('/bus/', function(data) {
            for (var i in data.busses) {
                var bus = data.busses[i];
                var elem = jQuery('<option>');
                elem.text(bus.name);
                elem.data('bus', bus);
                select.append(elem);
                if (initial_state.bus == bus.name) {
                    elem.attr('selected', 'true');
                    set_bus(bus);
                }
            }
            select.bind('change', function(e) {
                var elem = jQuery("option:selected", e.target);
                var b = elem.data('bus');
                if (b)
                    set_bus(b);
            });
        })
}

jQuery(document).ready(init);
