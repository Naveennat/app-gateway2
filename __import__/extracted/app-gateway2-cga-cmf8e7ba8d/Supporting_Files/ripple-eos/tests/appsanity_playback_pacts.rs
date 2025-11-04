use crate::client::xvp_playback::XvpPlayback;
use maplit::hashset;
use pact_consumer::prelude::*;
use pact_consumer::*;
use ripple_sdk::api::{
    firebolt::fb_discovery::{
        DataTagInfo, MediaEvent, MediaEventsAccountLinkRequestParams, ProgressUnit,
    },
    session::AccountSession,
};

#[tokio::test(flavor = "multi_thread")]
#[cfg_attr(not(feature = "http_contract_tests"), ignore)]
async fn test_playback_put_resume_points_pact() {
    let xvp_service = PactBuilder::new("ripple", "xvp-service")
    .interaction("When the user is Authorized in the system", "", |mut interaction_builder| {
        interaction_builder.given("When the user tries to put resume points");
        interaction_builder.request.method("PUT");
        interaction_builder.request.path(term!("/partners/[a-zA-Z0-9]*/accounts/[0-9]*/devices/[0-9]*/resumePoints/ott/[a-zA-Z0-9].*/[a-zA-Z0-9].*", "/partners/comcast/accounts/1234512345/deviceId/43434343/resumePoints/ott/partnerId0/contentId0"));
        interaction_builder.request.content_type("application/json;  charset=UTF-8");
        interaction_builder.request.header("authorization", term!("^Bearer [a-zA-Z0-9_.=]*$", "Bearer eyJraWQiOiJzYXQtcHJvZC1rMS0xMDI0IiwiYWxnIjoiUlMyNTYifQ.eyJwcmluY_lwYWwiOnsiZGdkIjpbTVEsMm0IMoXbF2mLrJZ6DmEQf7_YAe9U=="));
        interaction_builder.request.json_body(json_pattern!(
            {
                "durableAppId": like!("string"),
                "progress": like!(0),
                "progressUnits": like!("string"),
                "completed": like!(true),
                "ownerReference": term!("xrn:subscriber:device:[0-9].*", "xrn:subscriber:device:144468920343480805"),
                /*
                 * Really would like to add these fields. But these are optional fields and
                 * from ripple we are not filling them. And in pact there is no way to add
                 * optional field. (https://docs.pact.io/faq#why-is-there-no-support-for-specifying-optional-attributes)
                 * So when Ripple starts filling these fields we can enable
                 * these two fields
                 */

                // "correlationId": each_like!("string"),
                // "updated": each_like!(datetime!("yyyy-MM-dd'T'HH:mm:ss.SSS'Z'","2021-09-05T03:20:06.343Z"))
                /*
                 * These two fields are also optional but Ripple sends these fields
                 * hence adding them in the contract.
                 */
                "cet": each_like!(term!("dataPlatform:cet:xvp:[a-z].*[:a-z]","dataPlatform:cet:xvp:personalization:recommendation")),
                "cet_NotPropagated": each_like!(term!("dataPlatform:cet:xvp:[a-z].*[:a-z]","dataPlatform:cet:xvp:personalization:recommendation")),
                "categoryTags" : like!([])
            }
        ));
        interaction_builder.request.query_param("clientId", term!("^[0-9a-zA-Z]*$", "ripple"));
        interaction_builder.response
            .content_type("application/json")
            .status(200)
            .json_body(json_pattern!(
                {
                    "messageId": like!("msgId0"),
                    "snsStatusCode": like!(200),
                    "snsStatusText": like!("statusText123"),
                    "sequenceNumber": like!("134"),
                    "aws_request_id": like!("test_aws_request_ID")
                }
            ));
        interaction_builder}).start_mock_server(None);

    let xvp_url = xvp_service.path("");
    let distributor_session = AccountSession {
        id: String::from("comcast"),
        token: String::from("eyJraWQiOiJzYXQtcHJvZC1rMS0xMDI0IiwiYWxnIjoiUlMyNTYifQ.eyJwcmluY_lwYWwiOnsiZGdkIjpbTVEsMm0IMoXbF2mLrJZ6DmEQf7_YAe9U=="),
        account_id: String::from("6753750"),
        device_id: String::from("4523652"),
    };
    let media_event = MediaEvent {
        content_id: "ContentId001".to_owned(),
        completed: true,
        progress: 1.0,
        progress_unit: Some(ProgressUnit::Percent),
        watched_on: Some("2021-04-23T18:25:43.511Z".to_owned()),
        app_id: "Neflix".to_owned(),
    };

    let params = MediaEventsAccountLinkRequestParams {
        media_event,
        content_partner_id: "partner_id".to_owned(),
        client_supports_opt_out: false,
        dist_session: distributor_session,
        data_tags: hashset![
            DataTagInfo {
                tag_name: "dataPlatform:cet:xvp:personalization:recommendation".to_owned(),
                propagation_state: true
            },
            DataTagInfo {
                tag_name: "dataPlatform:cet:xvp:analytics".to_owned(),
                propagation_state: false
            },
        ],
        category_tags: vec![],
    };

    let result = XvpPlayback::put_resume_point(xvp_url.into(), params).await;
    assert!(result.is_ok());
}

#[tokio::test(flavor = "multi_thread")]
#[cfg_attr(not(feature = "http_contract_tests"), ignore)]
async fn test_unauth_playback_put_resume_points_pact() {
    let xvp_service = PactBuilder::new("ripple", "xvp-service")
    .interaction("When the user is NOT Authorized in the system", "", |mut interaction_builder| {
        interaction_builder.given("When the user tries to put resume points");
        interaction_builder.request.method("PUT");
        interaction_builder.request.path(term!("/partners/[a-zA-Z0-9]*/accounts/[0-9]*/devices/[0-9]*/resumePoints/ott/[a-zA-Z0-9].*/[a-zA-Z0-9].*", "/partners/comcast/accounts/1234512345/deviceId/43434343/resumePoints/ott/partnerId0/contentId0"));
        interaction_builder.request.content_type("application/json;  charset=UTF-8");
        interaction_builder.request.header("authorization", term!("^Bearer [a-zA-Z0-9_.=]*$", "Bearer eyJraWQiOiJzYXQtcHJvZC1rMS0xMDI0IiwiYWxnIjoiUlMyNTYifQ.eyJwcmluY_lwYWwiOnsiZGdkIjpbTVEsMm0IMoXbF2mLrJZ6DmEQf7_YAe9U=="));
        interaction_builder.request.json_body(json_pattern!(
            {
                "durableAppId": like!("string"),
                "progress": like!(0),
                "progressUnits": like!("string"),
                "completed": like!(true),
                "ownerReference": term!("xrn:subscriber:device:[0-9].*", "xrn:subscriber:device:144468920343480805"),
                /*
                 * Really would like to add these fields. But these are optional fields and
                 * from ripple we are not filling them. And in pact there is no way to add
                 * optional field. (https://docs.pact.io/faq#why-is-there-no-support-for-specifying-optional-attributes)
                 * So when Ripple starts filling these fields we can enable
                 * these two fields
                 */

                // "correlationId": each_like!("string"),
                // "updated": each_like!(datetime!("yyyy-MM-dd'T'HH:mm:ss.SSS'Z'","2021-09-05T03:20:06.343Z"))
                /*
                 * These two fields are also optional but Ripple sends these fields
                 * hence adding them in the contract.
                 */
                "cet": each_like!(term!("dataPlatform:cet:xvp:[a-z].*[:a-z]","dataPlatform:cet:xvp:personalization:recommendation")),
                "cet_NotPropagated": each_like!(term!("dataPlatform:cet:xvp:[a-z].*[:a-z]","dataPlatform:cet:xvp:personalization:recommendation")),
                "categoryTags": like!(["tag1,tag2"])
            }
        ));
        interaction_builder.request.query_param("clientId", term!("^[0-9a-zA-Z]*$", "ripple"));
        interaction_builder.response
            .content_type("application/json")
            .status(403);
        interaction_builder}).start_mock_server(None);

    let xvp_url = xvp_service.path("");
    let distributor_session = AccountSession {
        id: String::from("comcast"),
        token: String::from("eyJraWQiOiJzYXQtcHJvZC1rMS0xMDI0IiwiYWxnIjoiUlMyNTYifQ.eyJwcmluY_lwYWwiOnsiZGdkIjpbTVEsMm0IMoXbF2mLrJZ6DmEQf7_YAe9U=="),
        account_id: String::from("6753750"),
        device_id: String::from("4523652"),
    };
    let media_event = MediaEvent {
        content_id: "ContentId001".to_owned(),
        completed: true,
        progress: 1.0,
        progress_unit: Some(ProgressUnit::Percent),
        watched_on: Some("2021-04-23T18:25:43.511Z".to_owned()),
        app_id: "Neflix".to_owned(),
    };

    let params = MediaEventsAccountLinkRequestParams {
        media_event,
        content_partner_id: "partner_id".to_owned(),
        client_supports_opt_out: false,
        dist_session: distributor_session,
        data_tags: hashset![
            DataTagInfo {
                tag_name: "dataPlatform:cet:xvp:personalization:recommendation".to_owned(),
                propagation_state: true
            },
            DataTagInfo {
                tag_name: "dataPlatform:cet:xvp:analytics".to_owned(),
                propagation_state: false
            },
        ],
        category_tags: vec!["tag1".into(), "tag2".into()],
    };

    let result = XvpPlayback::put_resume_point(xvp_url.into(), params).await;
    assert!(result.is_err());
}
