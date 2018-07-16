package navitia
import io.gatling.core.Predef._
import io.gatling.http.Predef._


object Base {

  val httpConf = http
        .baseURL("http://localhost:5000/v1/coverage") //jormun
        .acceptEncodingHeader("gzip, deflate")

  def query(scn: String) = {
    val requests = csv("journeys_fr-idf.csv").circular
      exec().feed(requests).exec(
        http("${type}")
          .get("/${coverage}/journeys?${other_args}")
          .queryParam("from", "${from}")
          .queryParam("to", "${to}")
          .queryParam("datetime", "${datetime}")
          .queryParam("datetime_represents", "${datetime_represents}")
          .queryParam("_override_scenario", scn)
          .check(status.is(200)))
  }
}

class Distributed extends Simulation {
    setUp(scenario("distributed").exec(Base.query("distributed")).inject(constantUsersPerSec(1) during(300) randomized).protocols(Base.httpConf))
}

class NewDefault extends Simulation {
    setUp(scenario("new_default").exec(Base.query("new_default")).inject(constantUsersPerSec(1) during(300) randomized).protocols(Base.httpConf))
}
