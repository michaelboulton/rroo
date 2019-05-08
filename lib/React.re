type jsProps;

open Js_of_ocaml;

type reactElement;

type reactRef;

let null: reactElement = Js.Unsafe.js_expr("null");

let string: string => reactElement = s => Obj.magic(Js.string(s));

class type react = {
  pub createElement:
    (
      ReactClass.t,
      Js.Opt.t(Js.t({..})),
      Js.t(Js.js_array(reactElement))
    ) =>
    Js.meth(reactElement);
  pub cloneElement:
    (
      reactElement,
      Js.Opt.t(Js.t({..})),
      Js.t(Js.js_array(reactElement))
    ) =>
    Js.meth(reactElement);
};

let react: Js.t(react) = Js.Unsafe.global##.React;

external array: Js.t(Js.js_array(reactElement)) => reactElement =
  "%identity";

external refToJsObj: reactRef => Js.t({..}) = "%identity";

let createElement:
  (ReactClass.t, ~props: Js.t({..})=?, array(reactElement)) => reactElement =
  (reactClass, ~props=?, children) =>
    react##createElement(
      reactClass,
      Js.Opt.option(props),
      Js.array(children),
    );

let cloneElement:
  (reactElement, ~props: Js.t({..})=?, array(reactElement)) => reactElement =
  (reactElement, ~props=?, children) =>
    react##cloneElement(
      reactElement,
      Js.Opt.option(props),
      Js.array(children),
    );

/* let createElementVerbatim: 'a = react##createElement; */

let magicNull: 'a = Js.Unsafe.js_expr("null");

type reactClassInternal = ReactClass.t;

type renderNotImplemented =
  | RenderNotImplemented;

type stateless = unit;

type noRetainedProps = unit;

type actionless = unit;

/***
 * Elements are what JSX blocks become. They represent the *potential* for a
 * component instance and state to be created / updated. They are not yet
 * instances.
 */
type element =
  | Element(component('state, 'retainedProps, 'action)): element
and jsPropsToReason('jsProps, 'state, 'retainedProps, 'action) =
  'jsProps => component('state, 'retainedProps, 'action)
and uncurriedJsPropsToReason('jsProps, 'state, 'retainedProps, 'action) =
  (. 'jsProps) => component('state, 'retainedProps, 'action)
/***
 * Type of hidden field for Reason components that use JS components
 */
and jsElementWrapped =
  option(
    (~key: Js.Opt.t(string), ~ref: Js.Opt.t(Js.Opt.t(reactRef) => unit)) =>
    reactElement,
  )
and update('state, 'retainedProps, 'action) =
  | NoUpdate
  | Update('state)
  | SideEffects(self('state, 'retainedProps, 'action) => unit)
  | UpdateWithSideEffects(
      'state,
      self('state, 'retainedProps, 'action) => unit,
    )
/***
 * Granularly types state, and initial state as being independent, so that we
 * may include a template that all instances extend from.
 */
and componentSpec(
  'state,
  'initialState,
  'retainedProps,
  'initialRetainedProps,
  'action,
) = {
  debugName: string,
  reactClassInternal,
  /* Keep here as a way to prove that the API may be implemented soundly */
  mutable handedOffState: ref(option('state)),
  willReceiveProps: self('state, 'retainedProps, 'action) => 'state,
  didMount: self('state, 'retainedProps, 'action) => unit,
  didUpdate: oldNewSelf('state, 'retainedProps, 'action) => unit,
  willUnmount: self('state, 'retainedProps, 'action) => unit,
  willUpdate: oldNewSelf('state, 'retainedProps, 'action) => unit,
  shouldUpdate: oldNewSelf('state, 'retainedProps, 'action) => bool,
  render: self('state, 'retainedProps, 'action) => reactElement,
  initialState: unit => 'initialState,
  retainedProps: 'initialRetainedProps,
  reducer: ('action, 'state) => update('state, 'retainedProps, 'action),
  jsElementWrapped,
}
and component('state, 'retainedProps, 'action) =
  componentSpec('state, 'state, 'retainedProps, 'retainedProps, 'action)
and self('state, 'retainedProps, 'action) = {
  handle:
    'payload.
    (('payload, self('state, 'retainedProps, 'action)) => unit, 'payload) =>
    unit,

  state: 'state,
  retainedPropsSelf: 'retainedProps,
  send: 'action => unit,
  onUnmount: (unit => unit) => unit,
}
and oldNewSelf('state, 'retainedProps, 'action) = {
  oldSelf: self('state, 'retainedProps, 'action),
  newSelf: self('state, 'retainedProps, 'action),
};

type jsComponentThis('state, 'props, 'retainedProps, 'action) = {
  .
  "state": Js.readonly_prop(totalState('state, 'retainedProps, 'action)),
  "props": Js.readonly_prop({. "reasonProps": Js.readonly_prop('props)}),
  "setState":
    (
      (totalState('state, 'retainedProps, 'action), 'props) =>
      totalState('state, 'retainedProps, 'action),
      Js.Opt.t(unit => unit)
    ) =>
    Js.meth(unit),
  "jsPropsToReason":
    Js.readonly_prop(
      option(
        uncurriedJsPropsToReason('props, 'state, 'retainedProps, 'action),
      ),
    ),
}
/***
 * `totalState` tracks all of the internal reason API bookkeeping.
 *
 * Since we will mutate `totalState` in `shouldComponentUpdate`, and since
 * there's no guarantee that returning true from `shouldComponentUpdate`
 * guarantees that a component's update *actually* takes place (it could get
 * rolled back by Fiber etc), then we should put all properties that we
 * mutate directly on the totalState, so that when Fiber makes backup shallow
 * backup copies of `totalState`, our changes can be rolled back correctly
 * even when we mutate them.
 */
and totalState('state, 'retainedProps, 'action) = {
  .
  "reasonState": Js.readonly_prop('state),
};

let anyToUnit = _ => ();

let anyToTrue = _ => true;

let willReceivePropsDefault = ({state, _}) => state;

let renderDefault = _self => string("RenderNotImplemented");

let initialStateDefault = () => ();

let reducerDefault:
  ('action, 'state) => update('state, 'retainedProps, 'action) =
  (_action, _state) => NoUpdate;

let convertPropsIfTheyreFromJs = (props, jsPropsToReason, debugName) => {
  let props = Obj.magic(props);
  switch (Js.Opt.to_option(props##reasonProps), jsPropsToReason) {
  | (Some(props), _) => props
  | (None, Some(toReasonProps)) => Element(toReasonProps(. props))
  | (None, None) =>
    raise(
      Invalid_argument(
        "A JS component called the Reason component "
        ++ debugName
        ++ " which didn't implement the JS->Reason React props conversion.",
      ),
    )
  };
};

type reasonState_;
type retainedProps_;
type action_;

type reasonState_t =
  | ReasonState(reasonState_): reasonState_t;

type retainedProps_t =
  | RetainedProps(retainedProps_): retainedProps_t;

type action_t =
  | Action(action_): action_t;

let _createClass = ReactOptimizedCreateClass.createClass();
// let createClass =
//     (type reasonState, type retainedProps, type action, debugName): ReactClass.t =>
let createClass: string => ReactClass.t =
  debugName =>
    _createClass(
      [%js
        {
          /***
           * TODO: Null out fields that aren't overridden beyond defaults in
           * `component`. React optimizes components that don't implement
           * lifecycles!
           */
          val displayName = debugName;
          val mutable subscriptions = Js.null;
          /***
           * TODO: Avoid allocating this every time we need it. Should be doable.
           */
          pub selfRef = (state, retainedProps) => {
            handle: Obj.magic(Js.Unsafe.js_expr("this.handleMethod")),
            send: Obj.magic(Js.Unsafe.js_expr("this.sendMethod")),
            state,
            retainedPropsSelf: retainedProps,
            onUnmount: Obj.magic(Js.Unsafe.js_expr("this.onUnmountMethod")),
          };
          pub getInitialState =
              (): totalState('state, 'retainedProps, 'action) => {
            let thisJs:
              jsComponentThis(
                reasonState_t,
                element,
                retainedProps_t,
                action_t,
              ) =
              Js.Unsafe.js_expr("this");
            let convertedReasonProps =
              convertPropsIfTheyreFromJs(
                thisJs##.props,
                thisJs##.jsPropsToReason,
                debugName,
              );
            let Element(component) = convertedReasonProps;
            let initialReasonState = component.initialState();
            %js
            {val reasonState = Obj.magic(initialReasonState)};
          };
          pub componentDidMount = () => {
            let thisJs:
              jsComponentThis(
                reasonState_t,
                element,
                retainedProps_t,
                action_t,
              ) =
              Js.Unsafe.js_expr("this");
            let convertedReasonProps =
              convertPropsIfTheyreFromJs(
                thisJs##.props,
                thisJs##.jsPropsToReason,
                debugName,
              );
            let Element(component) = convertedReasonProps;
            let curTotalState = thisJs##.state;
            let curReasonState = curTotalState##.reasonState;
            let self =
              this##selfRef(
                curReasonState,
                Obj.magic(component.retainedProps),
              );
            let self = Obj.magic(self);
            if (component.didMount !== anyToUnit) {
              component.didMount(self);
            };
          };
          pub componentDidUpdate = (prevProps, prevState) => {
            let thisJs:
              jsComponentThis(
                reasonState_t,
                element,
                retainedProps_t,
                action_t,
              ) =
              Js.Unsafe.js_expr("this");
            let curState = thisJs##.state;
            let curReasonState = curState##.reasonState;
            let newJsProps = thisJs##.props;
            let newConvertedReasonProps =
              convertPropsIfTheyreFromJs(
                newJsProps,
                thisJs##.jsPropsToReason,
                debugName,
              );
            let Element(newComponent) = newConvertedReasonProps;
            if (newComponent.didUpdate !== anyToUnit) {
              let oldConvertedReasonProps =
                prevProps === newJsProps
                  ? newConvertedReasonProps
                  : convertPropsIfTheyreFromJs(
                      prevProps,
                      thisJs##.jsPropsToReason,
                      debugName,
                    );
              let Element(oldComponent) = oldConvertedReasonProps;
              let prevReasonState = prevState##reasonState;
              let prevReasonState = Obj.magic(prevReasonState);
              let newSelf =
                this##selfRef(
                  curReasonState,
                  Obj.magic(newComponent.retainedProps),
                );
              let newSelf = Obj.magic(newSelf);
              /* bypass this##selfRef call for small perf boost */
              let oldSelf =
                Obj.magic({
                  ...newSelf,
                  state: prevReasonState,
                  retainedPropsSelf: oldComponent.retainedProps,
                });
              newComponent.didUpdate({oldSelf, newSelf});
            };
          };
          /* pub componentWillMount .. TODO (or not?) */
          pub componentWillUnmount = () => {
            let thisJs:
              jsComponentThis(
                reasonState_t,
                element,
                retainedProps_t,
                action_t,
              ) =
              Js.Unsafe.js_expr("this");
            let convertedReasonProps =
              convertPropsIfTheyreFromJs(
                thisJs##.props,
                thisJs##.jsPropsToReason,
                debugName,
              );
            let Element(component) = convertedReasonProps;
            let curState = thisJs##.state;
            let curReasonState = curState##.reasonState;
            if (component.willUnmount !== anyToUnit) {
              let self =
                this##selfRef(
                  curReasonState,
                  Obj.magic(component.retainedProps),
                );
              let self = Obj.magic(self);
              component.willUnmount(self);
            };
            switch (Js.Opt.to_option(this##.subscriptions)) {
            | None => ()
            | Some(subs) =>
              subs##forEach(
                Js.wrap_callback((unsubscribe, _index, _array) =>
                  unsubscribe()
                ),
              )
            };
          };
          /***
           * If we are even getting this far, we've already done all the logic for
           * detecting unnecessary updates in shouldComponentUpdate. We know at
           * this point that we need to rerender, and we've even *precomputed* the
           * render result (subelements)!
           */
          pub componentWillUpdate = (nextProps, nextState: totalState(_)) => {
            let thisJs:
              jsComponentThis(
                reasonState_t,
                element,
                retainedProps_t,
                action_t,
              ) =
              Js.Unsafe.js_expr("this");
            let newConvertedReasonProps =
              convertPropsIfTheyreFromJs(
                nextProps,
                thisJs##.jsPropsToReason,
                debugName,
              );
            let Element(newComponent) = newConvertedReasonProps;
            if (newComponent.willUpdate !== anyToUnit) {
              let oldJsProps = thisJs##.props;
              /* Avoid converting again the props that are just the same as curProps. */
              let oldConvertedReasonProps =
                nextProps === oldJsProps
                  ? newConvertedReasonProps
                  : convertPropsIfTheyreFromJs(
                      oldJsProps,
                      thisJs##.jsPropsToReason,
                      debugName,
                    );
              let Element(oldComponent) = oldConvertedReasonProps;
              let curState = thisJs##.state;
              let curReasonState = curState##.reasonState;
              let curReasonState = Obj.magic(curReasonState);
              let nextReasonState = nextState##.reasonState;
              let newSelf =
                this##selfRef(
                  nextReasonState,
                  Obj.magic(newComponent.retainedProps),
                );
              let newSelf = Obj.magic(newSelf);
              /* bypass this##selfRef call for small perf boost */
              let oldSelf =
                Obj.magic({
                  ...newSelf,
                  state: curReasonState,
                  retainedPropsSelf: oldComponent.retainedProps,
                });
              newComponent.willUpdate({oldSelf, newSelf});
            };
          };
          /***
           * One interesting part of the new Reason React API. There isn't a need
           * for a separate `willReceiveProps` function. The primary `create` API
           * is *always* receiving props.
           */
          pub componentWillReceiveProps = nextProps => {
            let thisJs:
              jsComponentThis(
                reasonState_t,
                element,
                retainedProps_t,
                action_t,
              ) =
              Js.Unsafe.js_expr("this");
            let newConvertedReasonProps =
              convertPropsIfTheyreFromJs(
                nextProps,
                thisJs##.jsPropsToReason,
                debugName,
              );
            let Element(newComponent) = Obj.magic(newConvertedReasonProps);
            if (newComponent.willReceiveProps !== willReceivePropsDefault) {
              let oldJsProps = thisJs##.props;
              /* Avoid converting again the props that are just the same as curProps. */
              let oldConvertedReasonProps =
                nextProps === oldJsProps
                  ? newConvertedReasonProps
                  : convertPropsIfTheyreFromJs(
                      oldJsProps,
                      thisJs##.jsPropsToReason,
                      debugName,
                    );
              let Element(oldComponent) = oldConvertedReasonProps;
              thisJs##setState(
                (curTotalState, _) => {
                  let curReasonState = Obj.magic(curTotalState##.reasonState);
                  let oldSelf =
                    Obj.magic(
                      this##selfRef(
                        curReasonState,
                        Obj.magic(oldComponent.retainedProps),
                      ),
                    );
                  let nextReasonState =
                    Obj.magic(newComponent.willReceiveProps(oldSelf));
                  if (nextReasonState !== curTotalState) {
                    let nextTotalState: totalState(_) = [%js
                      {val reasonState = nextReasonState}
                    ];
                    let nextTotalState = Obj.magic(nextTotalState);
                    nextTotalState;
                  } else {
                    curTotalState;
                  };
                },
                Js.null,
              );
            };
          };
          /***
           * shouldComponentUpdate is invoked any time props change, or new state
           * updates occur.
           *
           * The easiest way to think about this method, is:
           * - "Should component have its componentWillUpdate method called,
           * followed by its render() method?",
           *
           * Therefore the component.shouldUpdate becomes a hook solely to perform
           * performance optimizations through.
           */
          pub shouldComponentUpdate = (nextJsProps, nextState, _) => {
            let thisJs:
              jsComponentThis(
                reasonState_t,
                element,
                retainedProps_t,
                action_t,
              ) =
              Js.Unsafe.js_expr("this");
            let curJsProps = thisJs##.props;

            /***
             * Now, we inspect the next state that we are supposed to render, and ensure that
             * - We have enough information to answer "should update?"
             * - We have enough information to render() in the event that the answer is "true".
             *
             * If we can detect that props have changed update has occured,
             * we ask the component's shouldUpdate if it would like to update - defaulting to true.
             */
            let oldConvertedReasonProps =
              convertPropsIfTheyreFromJs(
                thisJs##.props,
                thisJs##.jsPropsToReason,
                debugName,
              );
            /* Avoid converting again the props that are just the same as curProps. */
            let newConvertedReasonProps =
              nextJsProps === curJsProps
                ? oldConvertedReasonProps
                : convertPropsIfTheyreFromJs(
                    nextJsProps,
                    thisJs##.jsPropsToReason,
                    debugName,
                  );
            let Element(oldComponent) = oldConvertedReasonProps;
            let Element(newComponent) = newConvertedReasonProps;
            let nextReasonState = nextState##.reasonState;
            let newSelf =
              this##selfRef(
                nextReasonState,
                Obj.magic(newComponent.retainedProps),
              );
            if (newComponent.shouldUpdate !== anyToTrue) {
              let curState = thisJs##.state;
              let curReasonState = curState##.reasonState;
              let curReasonState = Obj.magic(curReasonState);
              let newSelf = Obj.magic(newSelf);
              /* bypass this##selfRef call for small perf boost */
              let oldSelf =
                Obj.magic({
                  ...newSelf,
                  state: curReasonState,
                  retainedPropsSelf: oldComponent.retainedProps,
                });
              newComponent.shouldUpdate({oldSelf, newSelf});
            } else {
              true;
            };
          };
          pub onUnmountMethod = subscription =>
            switch (Js.Opt.to_option(this##.subscriptions)) {
            | None =>
              this##.subscriptions :=
                Js.Opt.return(Js.array([|subscription|]))
            | Some(subs) => ignore(subs##push(subscription))
            };
          pub handleMethod = callback => {
            let thisJs:
              jsComponentThis(
                reasonState_t,
                element,
                retainedProps_t,
                action_t,
              ) =
              Js.Unsafe.js_expr("this");
            callbackPayload => {
              let curState = thisJs##.state;
              let curReasonState = curState##.reasonState;
              let convertedReasonProps =
                convertPropsIfTheyreFromJs(
                  thisJs##.props,
                  thisJs##.jsPropsToReason,
                  debugName,
                );
              let Element(component) = convertedReasonProps;
              callback(
                callbackPayload,
                Obj.magic(
                  this##selfRef(
                    curReasonState,
                    Obj.magic(component.retainedProps),
                  ),
                ),
              );
            };
          };
          pub sendMethod = (action: 'action) => {
            let thisJs:
              jsComponentThis(
                reasonState_t,
                element,
                retainedProps_t,
                action_t,
              ) =
              Js.Unsafe.js_expr("this");
            let convertedReasonProps =
              convertPropsIfTheyreFromJs(
                thisJs##.props,
                thisJs##.jsPropsToReason,
                debugName,
              );
            let Element(component) = convertedReasonProps;
            if (component.reducer !== reducerDefault) {
              let sideEffects = ref(ignore);
              /* allow side-effects to be executed here */
              let partialStateApplication =
                component.reducer(Obj.magic(action));
              thisJs##setState(
                (curTotalState, _) => {
                  let curReasonState = curTotalState##.reasonState;
                  let reasonStateUpdate =
                    partialStateApplication(Obj.magic(curReasonState));
                  if (reasonStateUpdate === NoUpdate) {
                    magicNull;
                  } else {
                    let reasonStateUpdate = Obj.magic(reasonStateUpdate);
                    let nextTotalState =
                      switch (reasonStateUpdate) {
                      | NoUpdate => curTotalState
                      | Update(nextReasonState) =>
                        %js
                        {val reasonState = nextReasonState}
                      | SideEffects(performSideEffects) =>
                        sideEffects.contents = performSideEffects;
                        curTotalState;
                      | UpdateWithSideEffects(
                          nextReasonState,
                          performSideEffects,
                        ) =>
                        sideEffects.contents = performSideEffects;
                        %js
                        {val reasonState = nextReasonState};
                      };
                    if (nextTotalState !== curTotalState) {
                      nextTotalState;
                    } else {
                      magicNull;
                    };
                  };
                },
                {
                  let cb = ((), self) => sideEffects.contents(self);
                  Js.Opt.return(
                    Js.Unsafe.meth_call(
                      this,
                      "handleMethod",
                      [|Js.Unsafe.inject(Js.wrap_callback(cb))|],
                    ),
                  );
                },
              );
            };
          };
          /***
           * In order to ensure we always operate on freshest props / state, and to
           * support the API that "reduces" the next state along with the next
           * rendering, with full acccess to named argument props in the closure,
           * we always *pre* compute the render result.
           */
          pub render = () => {
            let thisJs:
              jsComponentThis(
                reasonState_t,
                element,
                retainedProps_t,
                action_t,
              ) =
              Js.Unsafe.js_expr("this");
            let convertedReasonProps =
              convertPropsIfTheyreFromJs(
                thisJs##.props,
                thisJs##.jsPropsToReason,
                debugName,
              );
            let Element(created) = Obj.magic(convertedReasonProps);
            let component = created;
            let curState = thisJs##.state;
            let curReasonState = Obj.magic(curState##.reasonState);
            let self =
              Obj.magic(
                this##selfRef(
                  curReasonState,
                  Obj.magic(component.retainedProps),
                ),
              );
            component.render(self);
          }
        }
      ],
    );

let basicComponent = debugName => {
  let componentTemplate = {
    reactClassInternal: createClass(debugName),
    debugName,
    /* Keep here as a way to prove that the API may be implemented soundly */
    handedOffState: {
      contents: None,
    },
    didMount: anyToUnit,
    willReceiveProps: willReceivePropsDefault,
    didUpdate: anyToUnit,
    willUnmount: anyToUnit,
    willUpdate: anyToUnit,
    /***
     * Called when component will certainly mount at some point - and may be
     * called on the sever for server side React rendering.
     */
    shouldUpdate: anyToTrue,
    render: renderDefault,
    initialState: initialStateDefault,
    reducer: reducerDefault,
    jsElementWrapped: None,
    retainedProps: (),
  };
  componentTemplate;
};

let statelessComponent =
    (debugName): component(stateless, noRetainedProps, actionless) =>
  basicComponent(debugName);

let statelessComponentWithRetainedProps =
    (debugName)
    : componentSpec(
        stateless,
        stateless,
        'retainedProps,
        noRetainedProps,
        actionless,
      ) =>
  basicComponent(debugName);

let reducerComponent =
    (debugName)
    : componentSpec(
        'state,
        stateless,
        noRetainedProps,
        noRetainedProps,
        'action,
      ) =>
  basicComponent(debugName);

let reducerComponentWithRetainedProps =
    (debugName)
    : componentSpec(
        'state,
        stateless,
        'retainedProps,
        noRetainedProps,
        'action,
      ) =>
  basicComponent(debugName);

/***
 * Convenience for creating React elements before we have a better JSX transform.  Hopefully this makes it
 * usable to build some components while waiting to migrate the JSX transform to the next API.
 *
 * Constrain the component here instead of relying on the Element constructor which would lead to confusing
 * error messages.
 */
let element =
    (
      ~key: string=Obj.magic(Js.undefined),
      ~ref: Js.Opt.t(reactRef) => unit=Obj.magic(Js.undefined),
      component: component('state, 'retainedProps, 'action),
    ) => {
  let element = Element(component);
  switch (component.jsElementWrapped) {
  | Some(jsElementWrapped) =>
    jsElementWrapped(~key=Js.Opt.return(key), ~ref=Js.Opt.return(ref))
  | None =>
    createElement(
      component.reactClassInternal,
      ~props=[%js {val key = key; val ref = ref; val reasonProps = element}],
      [||],
    )
  };
};

let wrapReasonForJs =
    (
      ~component,
      jsPropsToReason:
        jsPropsToReason('jsProps, 'state, 'retainedProps, 'action),
    ) => {
  let jsPropsToReason:
    jsPropsToReason(jsProps, 'state, 'retainedProps, 'action) =
    Obj.magic(jsPropsToReason) /* cast 'jsProps to jsProps */;
  let uncurriedJsPropsToReason:
    uncurriedJsPropsToReason(jsProps, 'state, 'retainedProps, 'action) =
    (. jsProps) => jsPropsToReason(jsProps);
  Obj.magic(component.reactClassInternal)##prototype##jsPropsToReason :=
    Some(uncurriedJsPropsToReason);
  component.reactClassInternal;
};

/* module WrapProps = {
     /* We wrap the props for reason->reason components, as a marker that "these props were passed from another
        reason component" */
     let wrapProps =
         (
           ~reactClass,
           ~props,
           children,
           ~key: Js.Opt.t(string),
           ~ref: Js.Opt.t(Js.Opt.t(reactRef) => unit),
         ) => {
       let props =
         Js.Obj.assign(
           Js.Obj.assign(Js.Obj.empty(), Obj.magic(props)),
           {"ref": ref, "key": key},
         );
       let varargs =
         [|Obj.magic(reactClass), Obj.magic(props)|]
         |> Js.Array.concat(Obj.magic(children));
       /* Use varargs under the hood */
       Obj.magic(createElementVerbatim)##apply(Js.Nullable.null, varargs);
     };
     let dummyInteropComponent = basicComponent("interop");
     let wrapJsForReason =
         (~reactClass, ~props, children)
         : component(stateless, noRetainedProps, _) => {
       let jsElementWrapped = Some(wrapProps(~reactClass, ~props, children));
       {...dummyInteropComponent, jsElementWrapped};
     };
   };

   let wrapJsForReason = WrapProps.wrapJsForReason;

   [@bs.module "react"] external fragment: 'a = "Fragment";

   module Router = {
     [@bs.get] external location: Dom.window => Dom.location = "";

     [@bs.send]
     /* actually the cb is Dom.event => unit, but let's restrict the access for now */
     external addEventListener: (Dom.window, string, unit => unit) => unit = "";

     [@bs.send]
     external removeEventListener: (Dom.window, string, unit => unit) => unit =
       "";

     [@bs.send] external dispatchEvent: (Dom.window, Dom.event) => unit = "";

     [@bs.get] external pathname: Dom.location => string = "";

     [@bs.get] external hash: Dom.location => string = "";

     [@bs.get] external search: Dom.location => string = "";

     [@bs.send]
     external pushState:
       (Dom.history, [@bs.as {json|null|json}] _, [@bs.as ""] _, ~href: string) =>
       unit =
       "";

     [@bs.val] external event: 'a = "Event";

     [@bs.new] external makeEventIE11Compatible: string => Dom.event = "Event";

     [@bs.val] [@bs.scope "document"]
     external createEventNonIEBrowsers: string => Dom.event = "createEvent";

     [@bs.send]
     external initEventNonIEBrowsers: (Dom.event, string, bool, bool) => unit =
       "initEvent";

     let safeMakeEvent = eventName =>
       if (Js.typeof(event) == "function") {
         makeEventIE11Compatible(eventName);
       } else {
         let event = createEventNonIEBrowsers("Event");
         initEventNonIEBrowsers(event, eventName, true, true);
         event;
       };

     /* This is copied from array.ml. We want to cut dependencies for ReasonReact so
        that it's friendlier to use in size-constrained codebases */
     let arrayToList = a => {
       let rec tolist = (i, res) =>
         if (i < 0) {
           res;
         } else {
           tolist(i - 1, [Array.unsafe_get(a, i), ...res]);
         };
       tolist(Array.length(a) - 1, []);
     };
     /* if we ever roll our own parser in the future, make sure you test all url combinations
        e.g. foo.com/?#bar
        */
     /* sigh URLSearchParams doesn't work on IE11, edge16, etc. */
     /* actually you know what, not gonna provide search for now. It's a mess.
        We'll let users roll their own solution/data structure for now */
     let path = () =>
       switch ([%external window]) {
       | None => []
       | Some((window: Dom.window)) =>
         switch (window |> location |> pathname) {
         | ""
         | "/" => []
         | raw =>
           /* remove the preceeding /, which every pathname seems to have */
           let raw = Js.String.sliceToEnd(~from=1, raw);
           /* remove the trailing /, which some pathnames might have. Ugh */
           let raw =
             switch (Js.String.get(raw, Js.String.length(raw) - 1)) {
             | "/" => Js.String.slice(~from=0, ~to_=-1, raw)
             | _ => raw
             };
           raw |> Js.String.split("/") |> arrayToList;
         }
       };
     let hash = () =>
       switch ([%external window]) {
       | None => ""
       | Some((window: Dom.window)) =>
         switch (window |> location |> hash) {
         | ""
         | "#" => ""
         | raw =>
           /* remove the preceeding #, which every hash seems to have.
              Why is this even included in location.hash?? */
           raw |> Js.String.sliceToEnd(~from=1)
         }
       };
     let search = () =>
       switch ([%external window]) {
       | None => ""
       | Some((window: Dom.window)) =>
         switch (window |> location |> search) {
         | ""
         | "?" => ""
         | raw =>
           /* remove the preceeding ?, which every search seems to have. */
           raw |> Js.String.sliceToEnd(~from=1)
         }
       };
     let push = path =>
       switch ([%external history], [%external window]) {
       | (None, _)
       | (_, None) => ()
       | (Some((history: Dom.history)), Some((window: Dom.window))) =>
         pushState(history, ~href=path);
         dispatchEvent(window, safeMakeEvent("popstate"));
       };
     type url = {
       path: list(string),
       hash: string,
       search: string,
     };
     type watcherID = unit => unit;
     let url = () => {path: path(), hash: hash(), search: search()};
     /* alias exposed publicly */
     let dangerouslyGetInitialUrl = url;
     let watchUrl = callback =>
       switch ([%external window]) {
       | None => (() => ())
       | Some((window: Dom.window)) =>
         let watcherID = () => callback(url());
         addEventListener(window, "popstate", watcherID);
         watcherID;
       };
     let unwatchUrl = watcherID =>
       switch ([%external window]) {
       | None => ()
       | Some((window: Dom.window)) =>
         removeEventListener(window, "popstate", watcherID)
       };
   }; */
