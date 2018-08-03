package navitia
import io.gatling.core.Predef._
import io.gatling.http.Predef._

//------------------------------
// Base class
//-------------------------------
object Base {

  val httpConf = http
        .baseURL("http://localhost:5000/v1/coverage") //jormun
        .acceptEncodingHeader("gzip, deflate")

  def query(scn:String, data_set:String) = {
    val requests = csv(data_set).circular
      exec().feed(requests).exec(
        http("${type}")
          .get("/${coverage}/journeys?${other_args}")
          .queryParam("from", "${from}")
          .queryParam("to", "${to}")
          .queryParam("datetime", "${datetime}")
          .queryParam("datetime_represents", "${datetime_represents}")
          .queryParam("first_section_mode[]", "${first_section_mode}")
          .queryParam("last_section_mode[]", "${last_section_mode}")
          .queryParam("_override_scenario", scn)
          .check(status.is(200)))
  }
}

//-------------------------------
// Distributed Test Class
//-------------------------------
class Distributed extends Simulation {
    setUp(scenario("Distributed")
          .exec(Base.query("distributed", "fr-idf_request_models.csv"))
          .inject(constantUsersPerSec(1) during(500) randomized)
          .protocols(Base.httpConf))
}

//-------------------------------
// New Default Test Class
//-------------------------------
class NewDefault extends Simulation {
    setUp(scenario("NewDefault")
          .exec(Base.query("distributed", "fr-idf_request_models.csv"))
          .inject(constantUsersPerSec(1) during(500) randomized)
          .protocols(Base.httpConf))
}

