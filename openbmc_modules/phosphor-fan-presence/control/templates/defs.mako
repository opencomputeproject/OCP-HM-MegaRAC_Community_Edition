<%!
def indent(str, depth):
    return ''.join(4*' '*depth+line for line in str.splitlines(True))
%>\

<%def name="genParams(par)" buffered="True">
%if ('type' in par['hparams']) and \
    (par['hparams']['type'] is not None):
handler::${par['handler']}<${par['hparams']['type']}>(
%else:
handler::${par['handler']}(
%endif
%for i, hpk in enumerate(par['hparams']['params']):
%if (i+1) != len(par['hparams']['params']):
${par['hparams'][hpk]},
%else:
${par['hparams'][hpk]}
%endif
%endfor
)
</%def>\

<%def name="genSignal(sig)" buffered="True">
%if ('type' in sig['sparams']) and \
    (sig['sparams']['type'] is not None):
${indent(sig['signal'], 3)}<${sig['sparams']['type']}>(\
%else:
${indent(sig['signal'], 3)}(\
%endif
%for spk in sig['sparams']['params']:
${indent(sig['sparams'][spk], 3)},
%endfor
${genParams(par=sig)}
)
</%def>\

<%def name="genMethod(meth)" buffered="True">
%if ('type' in meth['mparams']) and \
    (meth['mparams']['type'] is not None):
${meth['method']}<${meth['mparams']['type']}>(
%else:
${meth['method']}(
%endif
%for spk in meth['mparams']['params']:
${meth['mparams'][spk]},
%endfor
${genParams(par=meth)}\
)
</%def>\

<%def name="genSSE(event)" buffered="True">
Group
{
%for group in event['groups']:
%for member in group['members']:
    {"${member['object']}",
    "${member['interface']}",
    "${member['property']}"},
%endfor
%endfor
},
ActionData{
%for e in event['action']:
{Group{
%for g in e['groups']:
%for m in g['members']:
    {"${m['object']}",
    "${m['interface']}",
    "${m['property']}"},
%endfor
%endfor
},
std::vector<Action>{
%for a in e['actions']:
%if len(a['parameters']) != 0:
make_action(action::${a['name']}(
%else:
make_action(action::${a['name']}
%endif
%for i, p in enumerate(a['parameters']):
%if (i+1) != len(a['parameters']):
    ${p},
%else:
    ${p}
%endif
%endfor
%if len(a['parameters']) != 0:
)),
%else:
),
%endif
%endfor
}},
%endfor
},
std::vector<Trigger>{
    %if ('timer' in event['triggers']) and \
        (event['triggers']['timer'] is not None):
    make_trigger(trigger::timer(TimerConf{
    ${event['triggers']['timer']['interval']},
    ${event['triggers']['timer']['type']}
    })),
    %endif
    %if ('signals' in event['triggers']) and \
        (event['triggers']['signals'] is not None):
    %for s in event['triggers']['signals']:
    make_trigger(trigger::signal(
        %if ('match' in s) and \
            (s['match'] is not None):
        match::${s['match']}(
        %for i, mp in enumerate(s['mparams']['params']):
        %if (i+1) != len(s['mparams']['params']):
        ${indent(s['mparams'][mp], 1)},
        %else:
        ${indent(s['mparams'][mp], 1)}
        %endif
        %endfor
        ),
        %else:
        "",
        %endif
        make_handler<SignalHandler>(\
        ${genSignal(sig=s)}
        )
    )),
    %endfor
    %endif
    %if ('init' in event['triggers']):
    %for i in event['triggers']['init']:
    make_trigger(trigger::init(
        %if ('method' in i):
        make_handler<MethodHandler>(\
        ${indent(genMethod(meth=i), 3)}\
        )
        %endif
    )),
    %endfor
    %endif
},
</%def>\
