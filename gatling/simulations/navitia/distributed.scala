package navitia
import io.gatling.core.Predef._
import io.gatling.http.Predef._


object Base {

  val httpConf = http
        .baseURL("http://localhost:5000/v1/coverage") //jormun
        .acceptEncodingHeader("gzip, deflate")

  def query(scn: String, first_mode: String, last_mode:String) = {
    val requests = csv("journeys_fr-idf.csv").circular
      exec().feed(requests).exec(
        http("${type}")
          .get("/${coverage}/journeys?${other_args}")
          .queryParam("from", "${from}")
          .queryParam("to", "${to}")
          .queryParam("datetime", "${datetime}")
          .queryParam("datetime_represents", "${datetime_represents}")
          .queryParam("first_section_mode[]", first_mode)
          .queryParam("last_section_mode[]", last_mode)
          .queryParam("_override_scenario", scn)
          .check(status.is(200)))
  }
}

class Distributed_Walking_Walking extends Simulation {
  setUp(scenario("Distributed_Walking_Walking").exec(Base.query("distributed", "walking", "walking"))
      .inject(constantUsersPerSec(1) during(300) randomized).protocols(Base.httpConf))
}

class Distributed_Walking_Bike extends Simulation {
    setUp(scenario("Distributed_Walking_Bike").exec(Base.query("distributed", "walking", "bike"))
      .inject(constantUsersPerSec(1) during(300) randomized).protocols(Base.httpConf))
}

class Distributed_Bike_Walking extends Simulation {
    setUp(scenario("Distributed_Bike_Walking").exec(Base.query("distributed", "bike", "walking"))
      .inject(constantUsersPerSec(1) during(300) randomized).protocols(Base.httpConf))
}

class Distributed_Car_Walking extends Simulation {
    setUp(scenario("Distributed_Car_Walking").exec(Base.query("distributed", "car", "walking"))
      .inject(constantUsersPerSec(1) during(300) randomized).protocols(Base.httpConf))
}

class Distributed_Walking_Car extends Simulation {
    setUp(scenario("Distributed_Walking_Car").exec(Base.query("distributed", "walking", "car"))
      .inject(constantUsersPerSec(1) during(300) randomized).protocols(Base.httpConf))
}

class NewDefault_Walking_Walking extends Simulation {
    setUp(scenario("NewDefault_Walking_Walking").exec(Base.query("new_default", "walking", "walking"))
      .inject(constantUsersPerSec(1) during(300) randomized).protocols(Base.httpConf))
}

class NewDefault_Walking_Bike extends Simulation {
    setUp(scenario("NewDefault_Walking_Bike").exec(Base.query("new_default", "walking", "bike"))
      .inject(constantUsersPerSec(1) during(300) randomized).protocols(Base.httpConf))
}

class NewDefault_Bike_Walking extends Simulation {
    setUp(scenario("NewDefault_Bike_Walking").exec(Base.query("new_default", "bike", "walking"))
      .inject(constantUsersPerSec(1) during(300) randomized).protocols(Base.httpConf))
}

class NewDefault_Car_Walking extends Simulation {
    setUp(scenario("NewDefault_Car_Walking").exec(Base.query("new_default", "car", "walking"))
      .inject(constantUsersPerSec(1) during(300) randomized).protocols(Base.httpConf))
}

class NewDefault_Walking_Car extends Simulation {
    setUp(scenario("NewDefault_Walking_Car").exec(Base.query("new_default", "walking", "car"))
      .inject(constantUsersPerSec(1) during(300) randomized).protocols(Base.httpConf))
}

